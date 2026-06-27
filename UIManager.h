#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "AudioManager.h"

class SDManager;  // Forward declaration

class UIManager
{
public:
    void begin();

    // Screen rendering methods
    void renderSplash();
    void renderTrackBrowser(SDManager &sdm, uint16_t totalTracks, int cursor, int scrollOffset, int displayLines);
    void update(const char *trackName, PlayState state,
                float progress, uint32_t posSec, uint32_t durSec,
                int volume, uint16_t trackIdx, uint16_t trackTotal);

private:
    Adafruit_SSD1306 _display{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
    unsigned long _lastRefresh = 0;

    // Marquee state for NOW_PLAYING screen
    int16_t _scrollX = 0;
    unsigned long _lastScroll = 0;
    const char *_prevTrackName = nullptr;

    // UI rendering helpers
    void drawTrackName(const char *name);
    void drawProgressBar(float progress, uint32_t posSec, uint32_t durSec);
    void drawControls(PlayState state, int volume);
    void formatTime(uint32_t sec, char *buf, size_t len);

    // External function to get track name from SDManager
    // (will be passed from AppController via SDManager)
    const char *getTrackName(uint16_t index);
};
