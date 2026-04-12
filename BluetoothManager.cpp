#include "BluetoothManager.h"
#include "driver/i2s.h"

static const uint8_t BT_DEVICE_NAME_MAX = 31;
static const uint32_t BT_PAIR_TIMEOUT_MS = 35000;

BluetoothManager *BluetoothManager::_self = nullptr;

void BluetoothManager::init()
{
    _self = this;

    _prefs.begin("btstore", false);
    loadPaired();

    // Configure sink callbacks, but DO NOT start at boot.
    // Starting A2DP at boot can claim I2S and block MP3 playback.
    _sink.set_auto_reconnect(true);
    _sink.set_on_connection_state_changed(onA2dpConnection, this);
    _sink.set_stream_reader(nullptr, false);

    setState(BT_IDLE, "BT: Ready");
}

bool BluetoothManager::ensureSinkRunning()
{
    if (_sinkStarted)
    {
        return true;
    }

    _sink.set_auto_reconnect(true);
    _sink.set_on_connection_state_changed(onA2dpConnection, this);
    _sink.set_stream_reader(nullptr, false);
    _sink.start(BT_FRIENDLY_NAME);
    _sinkStarted = true;

    // Ensure the Classic BT stack name matches the advertised sink name.
    esp_bt_dev_set_device_name(BT_FRIENDLY_NAME);

    if (!_gapRegistered)
    {
        esp_bt_gap_register_callback(onGapEvent);
        _gapRegistered = true;
    }

    configureSecurity();
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

#if DEBUG_SERIAL
    Serial.println(F("[BT] A2DP sink started"));
#endif
    return true;
}

void BluetoothManager::releaseI2S()
{
    if (!_sinkStarted)
    {
        return;
    }

#if DEBUG_SERIAL
    Serial.println(F("[BT] Releasing BT audio path"));
#endif

    // Aggressively release I2S so ESP8266Audio can reacquire it for MP3 playback.
    i2s_driver_uninstall(I2S_NUM_0);

    _sink.set_output_active(false);
    _sink.disconnect();
    delay(50);
    _sink.end();

    // Ensure the BT-owned I2S driver is fully gone before MP3 starts.
    i2s_driver_uninstall(I2S_NUM_0);
    delay(150);

    _sinkStarted = false;
    _connected = false;
    setState(BT_IDLE, "BT: Idle");
}

void BluetoothManager::update()
{
    unsigned long now = millis();

    if (_state == BT_AUTO_CONNECTING)
    {
        if (!_connected && (now - _stateStartMs) > BT_AUTO_CONNECT_TIMEOUT_MS)
        {
            setState(BT_FAILED, "BT: Failed");
        }
    }

    if (_state == BT_PAIRING)
    {
        if (!_connected && (now - _stateStartMs) > BT_PAIR_TIMEOUT_MS)
        {
            setState(BT_FAILED, "BT: Failed");
        }
    }

    if (_state == BT_SCANNING)
    {
        if ((now - _scanStartMs) > _scanTimeoutMs)
        {
            stopScan();
            if (_foundCount == 0)
            {
                setState(BT_FAILED, "No devices");
            }
            else
            {
                setState(BT_DEVICE_LIST, "Select device");
            }
        }
    }
}

bool BluetoothManager::connectPaired()
{
    if (!ensureSinkRunning())
    {
        setState(BT_FAILED, "BT init fail");
        return false;
    }

    loadPaired();
    if (_pairedMac.isEmpty())
    {
        return false;
    }

    esp_bd_addr_t bda = {0};
    if (!parseMac(_pairedMac, bda))
    {
        setState(BT_FAILED, "Bad MAC");
        return false;
    }

    _pendingName = _pairedName;
    _pendingMac = _pairedMac;

    setState(BT_AUTO_CONNECTING, "BT: Connecting");
    esp_err_t err = esp_a2d_sink_connect(bda);
    if (err != ESP_OK)
    {
        setState(BT_FAILED, "Connect fail");
        return false;
    }
    return true;
}

void BluetoothManager::startScan(uint32_t timeoutMs)
{
    if (!ensureSinkRunning())
    {
        setState(BT_FAILED, "BT init fail");
        return;
    }

    _foundCount = 0;
    _scanTimeoutMs = timeoutMs;
    _scanStartMs = millis();

    setState(BT_SCANNING, "Scanning...");

    // inq_len in units of 1.28 s → 0x14 = 20 × 1.28 = 25.6 s (≥ 20 s requirement)
    // num_rsps = 0 → unlimited responses
    esp_err_t err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x14, 0);
    if (err != ESP_OK)
    {
        setState(BT_FAILED, "Scan fail");
    }
}

void BluetoothManager::stopScan()
{
    esp_bt_gap_cancel_discovery();
}

bool BluetoothManager::connectToDevice(const String &name, const String &mac)
{
    if (!ensureSinkRunning())
    {
        setState(BT_FAILED, "BT init fail");
        return false;
    }

    const bool wasScanning = (_state == BT_SCANNING);

    esp_bd_addr_t bda = {0};
    if (!parseMac(mac, bda))
    {
        setState(BT_FAILED, "Bad MAC");
        return false;
    }

    _pendingName = name;
    _pendingMac = mac;

    setState(BT_PAIRING, "Pairing...");

    // Discovery/connect overlap can break pairing on some headsets (including Sony).
    if (wasScanning)
    {
        _connectAfterDiscoveryStop = true;
        stopScan();
        return true;
    }

    clearBondIfPresent(bda);

    esp_err_t err = esp_a2d_sink_connect(bda);
    if (err != ESP_OK)
    {
        setState(BT_FAILED, "Connect fail");
        return false;
    }

    return true;
}

void BluetoothManager::disconnect()
{
    if (_sinkStarted)
    {
        _sink.disconnect();
    }
    _connected = false;
    setState(BT_IDLE, "BT: Idle");
}

void BluetoothManager::stop()
{
    if (_state == BT_SCANNING)
    {
        stopScan();
    }

    releaseI2S();
#if DEBUG_SERIAL
    Serial.println(F("BT disconnected"));
#endif
}

BTState BluetoothManager::getState() const
{
    return _state;
}

bool BluetoothManager::isConnected() const
{
    return _connected;
}

bool BluetoothManager::isBusy() const
{
    return _state == BT_AUTO_CONNECTING || _state == BT_SCANNING || _state == BT_PAIRING;
}

bool BluetoothManager::isActive() const
{
    return _connected || isBusy();
}

const String &BluetoothManager::statusText() const
{
    return _status;
}

bool BluetoothManager::hasPairedDevice() const
{
    return !_pairedMac.isEmpty();
}

uint32_t BluetoothManager::scanElapsedSec() const
{
    if (_state != BT_SCANNING)
    {
        return 0;
    }
    return (millis() - _scanStartMs) / 1000;
}

uint8_t BluetoothManager::foundCount() const
{
    return _foundCount;
}

const BTDevice &BluetoothManager::foundDevice(uint8_t index) const
{
    static BTDevice empty = {"-", "", -127};
    if (index >= _foundCount)
    {
        return empty;
    }
    return _found[index];
}

void BluetoothManager::onGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    if (_self)
    {
        _self->handleGapEvent(event, param);
    }
}

void BluetoothManager::onA2dpConnection(esp_a2d_connection_state_t state, void *ctx)
{
    BluetoothManager *self = static_cast<BluetoothManager *>(ctx);
    if (self)
    {
        self->handleA2dpConnection(state);
    }
}

void BluetoothManager::setState(BTState s, const String &msg)
{
    _state = s;
    _status = msg;
    _stateStartMs = millis();
}

void BluetoothManager::loadPaired()
{
    _pairedName = _prefs.getString("paired_name", "");
    _pairedMac = _prefs.getString("paired_mac", "");
}

void BluetoothManager::savePaired(const String &name, const String &mac)
{
    _prefs.putString("paired_name", name);
    _prefs.putString("paired_mac", mac);
    _pairedName = name;
    _pairedMac = mac;
}

// Parse a device name from Extended Inquiry Response (EIR) TLV data.
// Type 0x08 = Shortened Local Name, 0x09 = Complete Local Name.
static String parseEirName(const uint8_t *eir, uint16_t len)
{
    uint16_t pos = 0;
    while (pos < len)
    {
        uint8_t field_len = eir[pos];
        if (field_len == 0 || (pos + field_len) >= len)
            break;
        uint8_t field_type = eir[pos + 1];
        if (field_type == 0x08 || field_type == 0x09)
        {
            uint8_t name_len = field_len - 1;
            if (name_len > BT_DEVICE_NAME_MAX)
                name_len = BT_DEVICE_NAME_MAX;
            char buf[BT_DEVICE_NAME_MAX + 1] = {0};
            memcpy(buf, &eir[pos + 2], name_len);
            return String(buf);
        }
        pos += (uint16_t)field_len + 1;
    }
    return String();
}

void BluetoothManager::handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    if (event == ESP_BT_GAP_DISC_RES_EVT)
    {
        String mac = macToString(param->disc_res.bda);

        // Skip duplicates
        for (uint8_t i = 0; i < _foundCount; i++)
        {
            if (_found[i].mac == mac)
                return;
        }

        String name;
        int rssi = -127;

        for (int i = 0; i < param->disc_res.num_prop; i++)
        {
            const esp_bt_gap_dev_prop_t *prop = &param->disc_res.prop[i];
            if (prop->type == ESP_BT_GAP_DEV_PROP_BDNAME && prop->len > 0)
            {
                // Legacy BD_NAME field
                name = String((const char *)prop->val).substring(0, BT_DEVICE_NAME_MAX);
            }
            else if (prop->type == ESP_BT_GAP_DEV_PROP_EIR && prop->len > 0 && name.isEmpty())
            {
                // Parse name from Extended Inquiry Response data
                name = parseEirName((const uint8_t *)prop->val, (uint16_t)prop->len);
            }
            else if (prop->type == ESP_BT_GAP_DEV_PROP_RSSI && prop->len >= 1)
            {
                rssi = *(const int8_t *)prop->val;
            }
        }

        if (name.isEmpty())
        {
            name = String("Device ") + (_foundCount + 1);
        }

        if (_foundCount < (sizeof(_found) / sizeof(_found[0])))
        {
            _found[_foundCount].name = name;
            _found[_foundCount].mac = mac;
            _found[_foundCount].rssi = rssi;
            _foundCount++;
        }
    }
    else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT)
    {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
        {
            if (_connectAfterDiscoveryStop)
            {
                _connectAfterDiscoveryStop = false;
                connectPendingDevice();
                return;
            }

            if (_state == BT_SCANNING)
            {
                if (_foundCount == 0)
                    setState(BT_FAILED, "No devices");
                else
                    setState(BT_DEVICE_LIST, "Select device");
            }
        }
    }
    else if (event == ESP_BT_GAP_CFM_REQ_EVT)
    {
        Serial.printf("[BT_PAIR] CFM_REQ from %s num_val=%u\n", macToString(param->cfm_req.bda).c_str(), param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        Serial.println(F("[BT_PAIR] CFM_REQ auto-accepted"));
    }
    else if (event == ESP_BT_GAP_PIN_REQ_EVT)
    {
        Serial.printf("[BT_PAIR] PIN_REQ from %s min_16_digit=%d\n", macToString(param->pin_req.bda).c_str(), param->pin_req.min_16_digit ? 1 : 0);
        esp_bt_pin_code_t pin_code;
        pin_code[0] = '0';
        pin_code[1] = '0';
        pin_code[2] = '0';
        pin_code[3] = '0';
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        Serial.println(F("[BT_PAIR] PIN reply sent (0000)"));
    }
    else if (event == ESP_BT_GAP_KEY_NOTIF_EVT)
    {
        Serial.printf("[BT_PAIR] KEY_NOTIF passkey=%06u from %s\n", param->key_notif.passkey, macToString(param->key_notif.bda).c_str());
    }
    else if (event == ESP_BT_GAP_AUTH_CMPL_EVT)
    {
        Serial.printf("[BT_PAIR] AUTH_CMPL status=%d device=%s (%s)\n",
                      (int)param->auth_cmpl.stat,
                      (const char *)param->auth_cmpl.device_name,
                      macToString(param->auth_cmpl.bda).c_str());
        if (!param->auth_cmpl.stat)
        {
            setState(BT_FAILED, "Pairing failed");
        }
        else
        {
            Serial.println(F("[BT_PAIR] Authentication complete"));
        }
    }
}

void BluetoothManager::handleA2dpConnection(esp_a2d_connection_state_t state)
{
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED)
    {
        _connected = true;
        setState(BT_CONNECTED, "BT: Connected");

        // Save as the single paired device (replaces any previous)
        if (!_pendingMac.isEmpty())
        {
            savePaired(_pendingName, _pendingMac);
        }
    }
    else if (state == ESP_A2D_CONNECTION_STATE_CONNECTING)
    {
        _connected = false;
        if (_state != BT_AUTO_CONNECTING)
        {
            setState(BT_PAIRING, "Pairing...");
        }
    }
    else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
    {
        _connected = false;
        _connectAfterDiscoveryStop = false;

        if (_state == BT_CONNECTED)
        {
            setState(BT_IDLE, "BT: Idle");
            return;
        }

        if (_state == BT_PAIRING || _state == BT_AUTO_CONNECTING)
        {
            setState(BT_FAILED, "Failed");
        }
        else
        {
            setState(BT_IDLE, "BT: Idle");
        }
    }
}

void BluetoothManager::configureSecurity()
{
    if (_securityConfigured)
    {
        return;
    }

    // Headset pairing works best with NoInputNoOutput on this hardware profile.
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

#if defined(ESP_BT_SP_AUTH_REQ) && defined(ESP_BT_SP_AUTH_REQ_AUTH)
    uint8_t authReq = ESP_BT_SP_AUTH_REQ_AUTH;
    esp_bt_gap_set_security_param((esp_bt_sp_param_t)ESP_BT_SP_AUTH_REQ, &authReq, sizeof(authReq));
#endif

    esp_bt_pin_type_t pinType = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pinCode;
    pinCode[0] = '0';
    pinCode[1] = '0';
    pinCode[2] = '0';
    pinCode[3] = '0';
    esp_bt_gap_set_pin(pinType, 4, pinCode);

    _securityConfigured = true;
}

bool BluetoothManager::connectPendingDevice()
{
    esp_bd_addr_t bda = {0};
    if (!parseMac(_pendingMac, bda))
    {
        setState(BT_FAILED, "Bad MAC");
        return false;
    }

    clearBondIfPresent(bda);

    esp_err_t err = esp_a2d_sink_connect(bda);
    if (err != ESP_OK)
    {
        setState(BT_FAILED, "Connect fail");
        return false;
    }

    return true;
}

void BluetoothManager::clearBondIfPresent(const esp_bd_addr_t bda)
{
    int devNum = esp_bt_gap_get_bond_device_num();
    if (devNum <= 0)
    {
        return;
    }

    esp_bd_addr_t *devList = new esp_bd_addr_t[devNum];
    if (!devList)
    {
        return;
    }

    if (esp_bt_gap_get_bond_device_list(&devNum, devList) == ESP_OK)
    {
        for (int i = 0; i < devNum; i++)
        {
            if (memcmp(devList[i], bda, sizeof(esp_bd_addr_t)) == 0)
            {
                esp_bt_gap_remove_bond_device(devList[i]);
                break;
            }
        }
    }

    delete[] devList;
}

bool BluetoothManager::parseMac(const String &mac, esp_bd_addr_t out)
{
    unsigned int b[6];
    int n = sscanf(mac.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    if (n != 6)
    {
        return false;
    }

    for (int i = 0; i < 6; i++)
    {
        out[i] = (uint8_t)b[i];
    }

    return true;
}

String BluetoothManager::macToString(const uint8_t *bda)
{
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return String(buf);
}
