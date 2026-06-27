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
    Serial.println(F("[UI] Starting I2C init..."));
    delay(250); // Allow display to power up before I2C init
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Serial.println(F("[UI] I2C initialized"));

    // Scan I2C bus to find what's connected
    Serial.println(F("[UI] Scanning I2C addresses..."));
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
        Serial.println(F("[UI] WARNING: No I2C devices found!"));

    Serial.println(F("[UI] Calling display.begin()..."));
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    {
        Serial.println(F("[UI] SSD1306 init failed (buffer alloc?)"));
        return;
    }
    Serial.println(F("[UI] Display buffer allocated"));
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);

    // Show static brand name while the rest of setup() runs
    const char *brand = "Rezvani Design";
    int16_t brandX = (OLED_WIDTH - (int16_t)(strlen(brand) * 6)) / 2;
    _display.setTextSize(1);
    _display.setCursor(brandX, 28);
    _display.print(brand);
    Serial.println(F("[UI] About to call display.display()..."));
    _display.display();
    Serial.println(F("[UI] Display initialized OK"));
}

// ── Splash Screen (SPLASH state) ──────────────────────────
void UIManager::renderSplash()
{
    _display.clearDisplay();
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);

    const char *title = "ESP32 MP3";
    const char *subtitle = "Player";

    int16_t titleX = (OLED_WIDTH - (int16_t)(strlen(title) * 12)) / 2;
    int16_t subtitleX = (OLED_WIDTH - (int16_t)(strlen(subtitle) * 12)) / 2;

    _display.setCursor(titleX, 20);
    _display.print(title);

    _display.setCursor(subtitleX, 40);
    _display.print(subtitle);

    _display.display();
}

// ── Track Browser Screen (TRACK_BROWSER state) ────────────
// Note: SDManager included via forward declaration in header
#include "SDManager.h"

void UIManager::renderTrackBrowser(SDManager &sdm, uint16_t totalTracks, int cursor, int scrollOffset, int displayLines)
{
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);

    const int PAD = 3;
    const int lineH = 10;
    const int startY = PAD + 8; // Account for header

    // Header
    _display.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);
    _display.setCursor(PAD, PAD);
    _display.print(F("TRACK LIST"));

    // Display tracks
    for (int i = 0; i < displayLines && (scrollOffset + i) < totalTracks; i++)
    {
        int trackIdx = scrollOffset + i;
        int y = startY + (i * lineH);

        // Highlight current cursor
        bool isSelected = (trackIdx == cursor);
        if (isSelected)
        {
            _display.fillRect(0, y - 1, OLED_WIDTH, lineH - 1, SSD1306_WHITE);
            _display.setTextColor(SSD1306_BLACK);
        }
        else
        {
            _display.setTextColor(SSD1306_WHITE);
        }

        // Format track entry: "NN. Filename"
        char buf[30];
        const char *trackName = sdm.getTrackName(trackIdx);
        snprintf(buf, sizeof(buf), "%02d. %s", trackIdx + 1, trackName ? trackName : "???");

        // Truncate if too long for display
        if (strlen(buf) > 21)
            buf[21] = '\0';

        _display.setCursor(PAD, y);
        _display.print(buf);
    }

    _display.display();
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
