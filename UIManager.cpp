#include "UIManager.h"

// ── Tiny 5×5 icons stored in PROGMEM ──────────────────────
// Prev icon: |◀◀  (8x8 bitmap)
static const uint8_t PROGMEM icon_prev[] = {
    0b00100100,
    0b01100110,
    0b11101110,
    0b01100110,
    0b00100100,
    0b00000000,
    0b00000000,
    0b00000000};

// Next icon: ▶▶|  (8x8 bitmap)
static const uint8_t PROGMEM icon_next[] = {
    0b00100100,
    0b01100110,
    0b01110111,
    0b01100110,
    0b00100100,
    0b00000000,
    0b00000000,
    0b00000000};

// Play icon: ▶  (8x8 bitmap)
static const uint8_t PROGMEM icon_play[] = {
    0b01000000,
    0b01100000,
    0b01110000,
    0b01111000,
    0b01110000,
    0b01100000,
    0b01000000,
    0b00000000};

// Pause icon: ▐▐  (8x8 bitmap)
static const uint8_t PROGMEM icon_pause[] = {
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b00000000};

// Stop icon: ■  (8x8 bitmap)
static const uint8_t PROGMEM icon_stop[] = {
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b01111110,
    0b00000000};

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
    _display.setTextSize(1);
    _display.setCursor(20, 12);
    _display.print(F("MP3 Player"));
    _display.display();
    Serial.println(F("[UI] Display initialized OK"));
}

void UIManager::update(const char *trackName, PlayState state,
                       float progress, uint32_t posSec, uint32_t durSec,
                       int volume, uint16_t trackIdx, uint16_t trackTotal)
{
    unsigned long now = millis();
    if (now - _lastRefresh < UI_REFRESH_MS)
        return;
    _lastRefresh = now;

    _display.clearDisplay();

    //  Row 1 (y=0..9): Track name with marquee
    drawTrackName(trackName);

    //  Row 2 (y=11..20): Progress bar + time
    drawProgressBar(progress, posSec, durSec);

    //  Row 3 (y=23..31): Transport controls + volume
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

    if (textW <= OLED_WIDTH)
    {
        // Fits — left align it
        _display.setCursor(0, 0);
        _display.print(name);
    }
    else
    {
        // Scrolling marquee
        unsigned long now = millis();
        if (now - _lastScroll >= SCROLL_STEP_MS)
        {
            _lastScroll = now;
            _scrollX--;
            // Reset after full scroll + small gap
            if (_scrollX < -(textW + 30))
            {
                _scrollX = OLED_WIDTH;
            }
        }
        _display.setCursor(_scrollX, 0);
        _display.print(name);
    }
}

// ── Row 2: Progress bar (80%) + time (20%) ────────────────
void UIManager::drawProgressBar(float progress, uint32_t posSec, uint32_t durSec)
{
    const int barY = 15;
    const int barH = 6;
    const int barW = (OLED_WIDTH * 80) / 100; // 80% of width = ~102px

    // Draw bar outline
    _display.drawRect(0, barY, barW, barH, SSD1306_WHITE);

    // Fill proportional
    int fillW = (int)(progress * (barW - 2));
    if (fillW > 0)
    {
        _display.fillRect(1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
    }

    // Time display in remaining 20%
    char posBuf[6];
    formatTime(posSec, posBuf, sizeof(posBuf));

    _display.setTextSize(1);
    _display.setCursor(barW + 2, barY);
    _display.print(posBuf);
}

// ── Row 3: Transport icons (left) + volume % (right) ──────
void UIManager::drawControls(PlayState state, int volume)
{
    const int y = 26;
    const int iconW = 8;
    const int spacing = 12;
    int startX = 0;

    // Prev
    _display.drawBitmap(startX, y, icon_prev, 8, 8, SSD1306_WHITE);

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
    _display.drawBitmap(startX + iconW + spacing, y, centerIcon, 8, 8, SSD1306_WHITE);

    // Next
    _display.drawBitmap(startX + (iconW + spacing) * 2, y, icon_next, 8, 8, SSD1306_WHITE);

    // Volume % at right end
    int volPct = (volume * 100) / VOLUME_MAX;
    char volBuf[5];
    snprintf(volBuf, sizeof(volBuf), "%d%%", volPct);
    int16_t volW = strlen(volBuf) * 6;
    _display.setTextSize(1);
    _display.setCursor(OLED_WIDTH - volW, y);
    _display.print(volBuf);
}

// ── Time formatter MM:SS ──────────────────────────────────
void UIManager::formatTime(uint32_t sec, char *buf, size_t len)
{
    uint32_t m = sec / 60;
    uint32_t s = sec % 60;
    snprintf(buf, len, "%u:%02u", m, s);
}
