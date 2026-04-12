#include "UIManager.h"

// ── 12×12 transport icons stored in PROGMEM ─────────────
// 2 bytes per row; pixel 0 = MSB of byte 0 (leftmost).

// Prev icon: |◀  vertical bar + left-pointing solid triangle
static const uint8_t PROGMEM icon_prev[] = {
    0x00, 0x00,  // row  0
    0xC1, 0xE0,  // row  1
    0xC3, 0xE0,  // row  2
    0xC7, 0xE0,  // row  3
    0xCF, 0xE0,  // row  4
    0xDF, 0xE0,  // row  5 (centre)
    0xCF, 0xE0,  // row  6
    0xC7, 0xE0,  // row  7
    0xC3, 0xE0,  // row  8
    0xC1, 0xE0,  // row  9
    0x00, 0x00,  // row 10
    0x00, 0x00}; // row 11

// Next icon: ▶|  exact horizontal mirror of icon_prev
static const uint8_t PROGMEM icon_next[] = {
    0x00, 0x00,  // row  0
    0x78, 0x30,  // row  1
    0x7C, 0x30,  // row  2
    0x7E, 0x30,  // row  3
    0x7F, 0x30,  // row  4
    0x7F, 0xB0,  // row  5 (centre)
    0x7F, 0x30,  // row  6
    0x7E, 0x30,  // row  7
    0x7C, 0x30,  // row  8
    0x78, 0x30,  // row  9
    0x00, 0x00,  // row 10
    0x00, 0x00}; // row 11

// Play icon: ▶  right-pointing solid triangle
static const uint8_t PROGMEM icon_play[] = {
    0x00, 0x00,  // row  0
    0x7C, 0x00,  // row  1
    0x7E, 0x00,  // row  2
    0x7F, 0x00,  // row  3
    0x7F, 0x80,  // row  4
    0x7F, 0xC0,  // row  5 (tip)
    0x7F, 0x80,  // row  6
    0x7F, 0x00,  // row  7
    0x7E, 0x00,  // row  8
    0x7C, 0x00,  // row  9
    0x00, 0x00,  // row 10
    0x00, 0x00}; // row 11

// Pause icon: ▐▐  two vertical bars
static const uint8_t PROGMEM icon_pause[] = {
    0x00, 0x00,  // row  0
    0x61, 0x80,  // row  1
    0x61, 0x80,  // row  2
    0x61, 0x80,  // row  3
    0x61, 0x80,  // row  4
    0x61, 0x80,  // row  5
    0x61, 0x80,  // row  6
    0x61, 0x80,  // row  7
    0x61, 0x80,  // row  8
    0x61, 0x80,  // row  9
    0x00, 0x00,  // row 10
    0x00, 0x00}; // row 11

// Stop icon: ■  solid rectangle
static const uint8_t PROGMEM icon_stop[] = {
    0x00, 0x00,  // row  0
    0x00, 0x00,  // row  1
    0x3F, 0xC0,  // row  2
    0x3F, 0xC0,  // row  3
    0x3F, 0xC0,  // row  4
    0x3F, 0xC0,  // row  5
    0x3F, 0xC0,  // row  6
    0x3F, 0xC0,  // row  7
    0x3F, 0xC0,  // row  8
    0x3F, 0xC0,  // row  9
    0x00, 0x00,  // row 10
    0x00, 0x00}; // row 11

void UIManager::begin()
{
    delay(250); // Allow display to power up before I2C init
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // Scan I2C bus — helps diagnose wrong address or bad wiring
    Serial.println(F("[UI] Scanning I2C bus..."));
    bool found = false;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.printf("[UI] Found device at 0x%02X\n", addr);
            found = true;
        }
    }
    if (!found)
        Serial.println(F("[UI] No I2C devices found! Check wiring."));

    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    {
        Serial.println(F("[UI] SSD1306 init failed (buffer alloc?)"));
        return;
    }
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);

    // Show static brand name while the rest of setup() runs.
    // The animated spinner will play during the first 3 s of loop().
    const char *brand = "Rezvani Design";
    int16_t brandX = (OLED_WIDTH - (int16_t)(strlen(brand) * 6)) / 2;
    _display.setTextSize(1);
    _display.setCursor(brandX, 28);
    _display.print(brand);
    _display.display();
    Serial.println(F("[UI] Display initialized OK"));
}

void UIManager::update(const char *trackName, PlayState state,
                       float progress, uint32_t posSec, uint32_t durSec,
                       int volume, uint16_t trackIdx, uint16_t trackTotal)
{
    unsigned long now = millis();
    // ── Splash phase: animate spinner for 3 s after first update() call ──
    if (!_splashStarted)
    {
        _splashStarted = true;
        _splashEndMs = now + 3000;
    }
    if (now < _splashEndMs)
    {
        if (now - _lastRefresh >= 125) // ~8 fps
        {
            _lastRefresh = now;
            const char *brand = "Rezvani Design";
            int16_t brandX = (OLED_WIDTH - (int16_t)(strlen(brand) * 6)) / 2;
            const char spinChars[] = {'|', '/', '-', '\\'};

            _display.clearDisplay();
            _display.setTextSize(2);
            _display.setCursor(58, 16);
            _display.print(spinChars[_spinFrame % 4]);
            _display.setTextSize(1);
            _display.setCursor(brandX, 40);
            _display.print(brand);
            _display.display();
            _spinFrame++;
        }
        return;
    }

    // ── Normal player UI ──────────────────────────────────────────────────
    if (now - _lastRefresh < UI_REFRESH_MS)
        return;
    _lastRefresh = now;

    _display.clearDisplay();

    //  Row 1 (y=0..11): Track name with marquee
    drawTrackName(trackName);

    //  Row 2 (y=22..31): Progress bar + time
    drawProgressBar(progress, posSec, durSec);

    //  Row 3 (y=46..53): Transport controls + volume
    drawControls(state, volume);

    _display.display();
}

// ── Row 1: Track name / scrolling marquee ─────────────────
void UIManager::drawTrackName(const char *name)
{
    // Reset scroll if track changed
    if (name != _prevTrackName)
    {
        _scrollX = 0;
        _lastScroll = millis();
        _prevTrackName = name;
    }

    _display.setTextSize(1); // 6px wide per char
    _display.setTextWrap(false);
    int16_t textW = strlen(name) * 6;

    const int PAD = 3;
    const int usableW = OLED_WIDTH - PAD * 2;

    if (textW <= usableW)
    {
        // Fits — left align within padded area
        _display.setCursor(PAD, PAD);
        _display.print(name);
    }
    else
    {
        // Scrolling marquee within padded area
        unsigned long now = millis();
        if (now - _lastScroll >= SCROLL_STEP_MS)
        {
            _lastScroll = now;
            _scrollX--;
            // Reset after full scroll + small gap
            if (_scrollX < -(textW + 30))
            {
                _scrollX = PAD + usableW;
            }
        }
        _display.setCursor(PAD + _scrollX, PAD);
        _display.print(name);
    }
}

// ── Row 2: Timers above + full-width progress bar ─────────
void UIManager::drawProgressBar(float progress, uint32_t posSec, uint32_t durSec)
{
    const int PAD = 3;
    const int timerY = 22; // timers row
    const int barY = 33;   // progress bar
    const int barH = 8;
    const int barX = PAD;                  // padded left edge
    const int barW = OLED_WIDTH - PAD * 2; // padded width

    // Elapsed time — left
    char posBuf[6];
    formatTime(posSec, posBuf, sizeof(posBuf));
    _display.setTextSize(1);
    _display.setCursor(PAD, timerY);
    _display.print(posBuf);

    // Remaining time — right
    uint32_t remSec = (durSec > posSec) ? (durSec - posSec) : 0;
    char remBuf[6];
    formatTime(remSec, remBuf, sizeof(remBuf));
    int16_t remW = strlen(remBuf) * 6;
    _display.setCursor(OLED_WIDTH - PAD - remW, timerY);
    _display.print(remBuf);

    // Draw bar outline
    _display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);

    // Fill proportional
    int fillW = (int)(progress * (barW - 2));
    if (fillW > 0)
    {
        _display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
    }
}

// ── Row 3: Transport icons (left) + volume (right) ──────
void UIManager::drawControls(PlayState state, int volume)
{
    const int PAD = 3;
    const int y = 49; // 49 + 12 = 61, leaving 3px bottom padding
    const int iconW = 12;
    const int spacing = 10;
    int startX = PAD; // 3px left padding

    // Prev
    _display.drawBitmap(startX, y, icon_prev, 12, 12, SSD1306_WHITE);

    // Play/Pause/Stop
    const uint8_t *centerIcon;
    switch (state)
    {
    case PlayState::PLAYING:
        centerIcon = icon_pause;
        break;
    case PlayState::PAUSED:
        centerIcon = icon_play;
        break;
    default:
        centerIcon = icon_play; // STOPPED: show play icon (ready to play)
        break;
    }
    _display.drawBitmap(startX + iconW + spacing, y, centerIcon, 12, 12, SSD1306_WHITE);

    // Next
    _display.drawBitmap(startX + (iconW + spacing) * 2, y, icon_next, 12, 12, SSD1306_WHITE);

    // Volume at right end with right padding
    char volBuf[8];
    snprintf(volBuf, sizeof(volBuf), "vol:%d", volume);
    int16_t volW = strlen(volBuf) * 6;
    _display.setTextSize(1);
    _display.setCursor(OLED_WIDTH - PAD - volW, y + 2);
    _display.print(volBuf);
}

// ── Time formatter MM:SS ──────────────────────────────────
void UIManager::formatTime(uint32_t sec, char *buf, size_t len)
{
    uint32_t m = sec / 60;
    uint32_t s = sec % 60;
    snprintf(buf, len, "%u:%02u", m, s);
}

// ============================================================
//  BLUETOOTH MENU  (128 × 64 display)
//
//  Layout on 64-px tall screen:
//    y =  0-10 → title bar (filled rect, white-on-black text)
//    y = 13    → item row 0
//    y = 23    → item row 1
//    y = 33    → item row 2
//    y = 43    → item row 3
//    y = 53    → item row 4
//  Selected row is highlighted with a filled rect.
//  Up to 5 items visible at once → no scrolling needed for
//  the main menu; device lists scroll with _btFoundSel.
// ============================================================

// ── Internal helpers ──────────────────────────────────────

// Draw a full-width inverted title bar at the top.
void UIManager::drawBtTitle(const char *title)
{
    _display.fillRect(0, 0, OLED_WIDTH, 11, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setCursor(3, 2);
    _display.print(title);
    _display.setTextColor(SSD1306_WHITE); // restore for items
}

// Draw a single menu item row.
// row: 0-4  →  y = 13, 23, 33, 43, 53
// selected: draws filled highlight rect behind text
void UIManager::drawBtItem(int row, bool selected, const char *text)
{
    const int y = 13 + row * 10;
    if (selected)
    {
        _display.fillRect(0, y - 1, OLED_WIDTH, 10, SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK);
    }
    else
    {
        _display.setTextColor(SSD1306_WHITE);
    }
    _display.setTextSize(1);
    _display.setCursor(4, y);
    _display.print(text);
    _display.setTextColor(SSD1306_WHITE); // always restore
}

String UIManager::clipStr(const String &s, uint8_t maxCh) const
{
    if (s.length() <= maxCh)
        return s;
    return s.substring(0, maxCh - 1) + '.';
}

int UIManager::wrapIdx(int i, int total)
{
    if (total <= 0)
        return 0;
    while (i < 0)
        i += total;
    while (i >= total)
        i -= total;
    return i;
}

// ── Public BT menu state control ─────────────────────────

void UIManager::openBtMenu()
{
    _btOpen = true;
    _btView = UIBTView::MAIN;
    _btMainSel = 0;
}

void UIManager::closeBtMenu()
{
    _btOpen = false;
}

bool UIManager::isBtMenuOpen() const
{
    return _btOpen;
}

// Called by AppController every loop() to let view transitions
// react to BT state changes (e.g. scan finishing, connect succeeding).
void UIManager::notifyBtState(const BluetoothManager &bt)
{
    BTState s = bt.getState();
    if (s == _prevBtState)
        return;
    _prevBtState = s;

    if (!_btOpen)
        return;

    if (s == BT_DEVICE_LIST)
    {
        // Transition from scanning → device list view
        if (bt.foundCount() == 1)
            _btView = UIBTView::SINGLE_FOUND;
        else if (bt.foundCount() > 1)
            _btView = UIBTView::FOUND_LIST;
        else
            _btView = UIBTView::FAIL_OPTIONS; // 0 devices
        _btFoundSel = 0;
    }
    else if (s == BT_FAILED)
    {
        _btView = UIBTView::FAIL_OPTIONS;
        _btFailSel = 0;
    }
    else if (s == BT_CONNECTED)
    {
        _btView = UIBTView::MAIN; // return to main after connect
    }
    else if (s == BT_SCANNING)
    {
        _btView = UIBTView::SCANNING;
    }
    else if (s == BT_PAIRING)
    {
        _btView = UIBTView::PAIRING;
    }
}

// ── Rotary input while menu is open ──────────────────────

void UIManager::onBtRotate(int dir, const BluetoothManager &bt)
{
    if (!_btOpen)
        return;

    switch (_btView)
    {
    case UIBTView::MAIN:
        _btMainSel = wrapIdx(_btMainSel + dir, 3);
        break;
    case UIBTView::FOUND_LIST:
        _btFoundSel = wrapIdx(_btFoundSel + dir, (int)bt.foundCount());
        break;
    case UIBTView::SINGLE_FOUND:
        _btFoundSel = wrapIdx(_btFoundSel + dir, 2); // Connect / Rescan
        break;
    case UIBTView::FAIL_OPTIONS:
        _btFailSel = wrapIdx(_btFailSel + dir, 3); // Retry/Rescan/Back
        break;
    default:
        break;
    }
}

// ── Click (short press) while menu is open ──────────────

UIAction UIManager::onBtClick(const BluetoothManager &bt)
{
    UIAction action;
    if (!_btOpen)
        return action;

    switch (_btView)
    {
    case UIBTView::MAIN:
        switch (_btMainSel)
        {
        case 0: /* Status — info only, no action  */
            break;
        case 1:
            action.type = UIActionType::START_SCAN;
            break;
        case 2:
            _btOpen = false;
            break;
        }
        break;

    case UIBTView::FOUND_LIST:
        if (bt.foundCount() > 0)
        {
            action.type = UIActionType::CONNECT_FOUND;
            action.index = _btFoundSel;
        }
        break;

    case UIBTView::SINGLE_FOUND:
        if (_btFoundSel == 0)
        {
            action.type = UIActionType::CONNECT_FOUND;
            action.index = 0;
        }
        else
        {
            action.type = UIActionType::RESCAN;
        }
        break;

    case UIBTView::FAIL_OPTIONS:
        switch (_btFailSel)
        {
        case 0:
            action.type = UIActionType::RETRY;
            break;
        case 1:
            action.type = UIActionType::RESCAN;
            break;
        case 2:
            _btView = UIBTView::MAIN;
            break;
        }
        break;

    default:
        break;
    }

    return action;
}

// ── Long press while menu is open → go back ──────────────

void UIManager::onBtLongPress(const BluetoothManager &bt)
{
    if (!_btOpen)
    {
        openBtMenu();
        return;
    }

    switch (_btView)
    {
    case UIBTView::MAIN:
        _btOpen = false;
        break;
    case UIBTView::SCANNING:
    case UIBTView::FOUND_LIST:
    case UIBTView::SINGLE_FOUND:
    case UIBTView::PAIRING:
    case UIBTView::FAIL_OPTIONS:
        _btView = UIBTView::MAIN;
        _btMainSel = 0;
        break;
    }
}

// ── Rendering ────────────────────────────────────────────

void UIManager::renderBluetooth(const BluetoothManager &bt)
{
    if (!_btOpen)
        return;

    _display.clearDisplay();

    switch (_btView)
    {
    // ── Main menu ───────────────────────────────────────
    case UIBTView::MAIN:
    {
        drawBtTitle("Bluetooth");
        const char *items[3] = {
            bt.isConnected() ? "Status: Connected" : "Status: No device",
            "Pair New Device",
            "Exit"};
        for (int i = 0; i < 3; i++)
            drawBtItem(i, i == _btMainSel, items[i]);
    }
    break;

    // ── Scanning ────────────────────────────────────────
    case UIBTView::SCANNING:
    {
        drawBtTitle("Scanning...");
        char buf[22];
        snprintf(buf, sizeof(buf), "Elapsed: %us",
                 (unsigned)bt.scanElapsedSec());
        drawBtItem(0, false, "Put device in pair");
        drawBtItem(1, false, "mode, then wait...");
        drawBtItem(2, false, buf);
        drawBtItem(3, false, "LongPress: cancel");
    }
    break;

    // ── Multiple devices found ──────────────────────────
    case UIBTView::FOUND_LIST:
    {
        drawBtTitle("Select Device");
        uint8_t cnt = bt.foundCount();
        int start = (_btFoundSel > 3) ? _btFoundSel - 3 : 0;
        for (int row = 0; row < 4 && (start + row) < cnt; row++)
        {
            int idx = start + row;
            String name = clipStr(bt.foundDevice(idx).name, 20);
            drawBtItem(row, idx == _btFoundSel, name.c_str());
        }
        // Scroll hint
        if (cnt > 4)
        {
            char hint[16];
            snprintf(hint, sizeof(hint), "%d/%d", _btFoundSel + 1, cnt);
            _display.setTextColor(SSD1306_WHITE);
            _display.setTextSize(1);
            _display.setCursor(100, 54);
            _display.print(hint);
        }
    }
    break;

    // ── Single device found ─────────────────────────────
    case UIBTView::SINGLE_FOUND:
    {
        drawBtTitle("Device Found");
        if (bt.foundCount() > 0)
            drawBtItem(0, false, clipStr(bt.foundDevice(0).name, 20).c_str());
        drawBtItem(1, _btFoundSel == 0, "Connect");
        drawBtItem(2, _btFoundSel == 1, "Rescan");
        drawBtItem(3, false, "LongPress: back");
    }
    break;

    // ── Pairing in progress ─────────────────────────────
    case UIBTView::PAIRING:
    {
        drawBtTitle("Pairing...");
        drawBtItem(0, false, bt.statusText().c_str());
        drawBtItem(2, false, "Please wait");
    }
    break;

    // ── Failure options ─────────────────────────────────
    case UIBTView::FAIL_OPTIONS:
    {
        drawBtTitle("BT: Failed");
        drawBtItem(0, false, bt.statusText().c_str());
        drawBtItem(1, _btFailSel == 0, "Retry");
        drawBtItem(2, _btFailSel == 1, "Rescan");
        drawBtItem(3, _btFailSel == 2, "Back");
    }
    break;
    }

    _display.display();
}
