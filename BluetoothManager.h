#pragma once

#include <Arduino.h>
#include <BluetoothA2DPSink.h>
#include <Preferences.h>
#include <esp_a2dp_api.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include "Config.h"

enum BTState : uint8_t
{
    BT_IDLE,
    BT_AUTO_CONNECTING,
    BT_CONNECTED,
    BT_SCANNING,
    BT_DEVICE_LIST,
    BT_PAIRING,
    BT_FAILED
};

struct BTDevice
{
    String name;
    String mac;
    int rssi = -127;
};

class BluetoothManager
{
public:
    void init();
    void update();

    bool ensureSinkRunning();
    void releaseI2S();

    bool connectPaired();
    void startScan(uint32_t timeoutMs = BT_SCAN_TIMEOUT_MS);
    void stopScan();
    bool connectToDevice(const String &name, const String &mac);
    void disconnect();
    void stop();

    BTState getState() const;
    bool isConnected() const;
    bool isBusy() const;
    bool isActive() const;
    const String &statusText() const;
    bool hasPairedDevice() const;

    uint32_t scanElapsedSec() const;
    uint8_t foundCount() const;
    const BTDevice &foundDevice(uint8_t index) const;

private:
    static BluetoothManager *_self;
    static void onGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static void onA2dpConnection(esp_a2d_connection_state_t state, void *ctx);

    void setState(BTState s, const String &msg);
    void loadPaired();
    void savePaired(const String &name, const String &mac);
    void configureSecurity();
    bool connectPendingDevice();
    void clearBondIfPresent(const esp_bd_addr_t bda);
    void handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    void handleA2dpConnection(esp_a2d_connection_state_t state);

    static bool parseMac(const String &mac, esp_bd_addr_t out);
    static String macToString(const uint8_t *bda);

    BluetoothA2DPSink _sink;
    Preferences _prefs;

    BTState _state = BT_IDLE;
    bool _connected = false;
    String _status = "BT: Idle";

    unsigned long _stateStartMs = 0;
    unsigned long _scanStartMs = 0;
    uint32_t _scanTimeoutMs = BT_SCAN_TIMEOUT_MS;

    BTDevice _found[8];
    uint8_t _foundCount = 0;

    String _pairedName;
    String _pairedMac;

    String _pendingName;
    String _pendingMac;

    bool _sinkStarted = false;
    bool _gapRegistered = false;
    bool _securityConfigured = false;
    bool _connectAfterDiscoveryStop = false;
};
