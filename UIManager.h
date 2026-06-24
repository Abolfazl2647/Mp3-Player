#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "AudioManager.h"

class UIManager
{
public:
    void begin();
    void update(const char *trackName, PlayState state,
                float progress, uint32_t posSec, uint32_t durSec,
                int volume, uint16_t trackIdx, uint16_t trackTotal);

private:
    Adafruit_SSD1306 _display{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
    unsigned long _lastRefresh = 0;

    // Splash state
    bool _splashStarted = false;
    unsigned long _splashEndMs = 0;
    uint8_t _spinFrame = 0;

    // Marquee state
    int16_t _scrollX = 0;
    unsigned long _lastScroll = 0;
    const char *_prevTrackName = nullptr;

    void drawTrackName(const char *name);
    void drawProgressBar(float progress, uint32_t posSec, uint32_t durSec);
    void drawControls(PlayState state, int volume);
    void formatTime(uint32_t sec, char *buf, size_t len);
};
