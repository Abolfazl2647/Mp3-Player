#pragma once
#include <Arduino.h>
#include "Config.h"

// Callback types for input events
typedef void (*ButtonCallback)();
typedef void (*MenuCallback)(int direction); // +1 for down, -1 for up

class InputManager
{
public:
    void begin();
    void update(); // call from loop()

    // Menu navigation callbacks
    void onMenuUp(ButtonCallback cb);
    void onMenuDown(ButtonCallback cb);
    void onMenuSelect(ButtonCallback cb);

    // Playback control callbacks
    void onPrevTrack(ButtonCallback cb);
    void onNextTrack(ButtonCallback cb);
    void onPlayPause(ButtonCallback cb);

    // Volume callbacks
    void onVolumeUp(ButtonCallback cb);
    void onVolumeDown(ButtonCallback cb);

    // Long-press detection for menu select button
    bool isSelectLongPressed() const;

private:
    // Debounced button state
    struct Button
    {
        uint8_t pin;
        bool lastStable;
        bool lastReading;
        unsigned long lastChangeMs;
        ButtonCallback callback;
    };

    // 5-way navigation module buttons
    Button _btn5wayU;    // Menu up
    Button _btn5wayD;    // Menu down
    Button _btn5wayL;    // Prev track
    Button _btn5wayR;    // Next track
    Button _btn5wayC;    // Menu select / Play-pause

    // Volume buttons
    Button _btnVolUp;
    Button _btnVolDown;

    // Long-press detection for 5-way C button
    struct LongPressState
    {
        unsigned long pressStartMs = 0;
        bool isPressed = false;
    } _selectLongPress;

    const unsigned long LONG_PRESS_MS = 1500; // 1.5 seconds

    void initButton(Button &btn, uint8_t pin);
    void updateButton(Button &btn);
    void updateButtonWithLongPress(Button &btn, LongPressState &longPress);
};
