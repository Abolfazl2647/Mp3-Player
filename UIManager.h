#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "AudioManager.h"
#include "BluetoothManager.h"

// ── BT menu action returned to AppController ─────────────
enum class UIActionType : uint8_t
{
    NONE,
    START_SCAN,
    CONNECT_FOUND,
    RESCAN,
    RETRY
};

struct UIAction
{
    UIActionType type = UIActionType::NONE;
    int index = -1; // device index where relevant
};

// ── BT menu sub-views ─────────────────────────────────────
enum class UIBTView : uint8_t
{
    MAIN,
    SCANNING,
    FOUND_LIST,
    SINGLE_FOUND,
    PAIRING,
    FAIL_OPTIONS
};

class UIManager
{
public:
    void begin();
    void update(const char *trackName, PlayState state,
                float progress, uint32_t posSec, uint32_t durSec,
                int volume, uint16_t trackIdx, uint16_t trackTotal);

    // ── BT menu interface ─────────────────────────────────
    void openBtMenu();  // long press opens menu
    void closeBtMenu(); // long press from MAIN closes
    bool isBtMenuOpen() const;
    void onBtRotate(int dir, const BluetoothManager &bt);
    UIAction onBtClick(const BluetoothManager &bt);
    void onBtLongPress(const BluetoothManager &bt);
    void renderBluetooth(const BluetoothManager &bt); // called every loop
    void notifyBtState(const BluetoothManager &bt);   // drives view transitions

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

    // ── BT menu state ──────────────────────────────────────
    bool _btOpen = false;
    UIBTView _btView = UIBTView::MAIN;
    int _btMainSel = 0; // 0-2
    int _btFoundSel = 0;
    int _btFailSel = 0; // Retry/Rescan/Back
    BTState _prevBtState = BT_IDLE;

    // ── MP3 player helpers ────────────────────────────────
    void drawTrackName(const char *name);
    void drawProgressBar(float progress, uint32_t posSec, uint32_t durSec);
    void drawControls(PlayState state, int volume);
    void formatTime(uint32_t sec, char *buf, size_t len);

    // ── BT menu helpers ───────────────────────────────────
    void drawBtTitle(const char *title);
    void drawBtItem(int row, bool selected, const char *text);
    String clipStr(const String &s, uint8_t maxCh) const;
    static int wrapIdx(int i, int total);
};
