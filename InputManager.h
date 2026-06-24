#pragma once
#include <Arduino.h>
#include "Config.h"

// Callback types for input events
typedef void (*ButtonCallback)();
typedef void (*EncoderCallback)(int direction); // +1 or -1

class InputManager
{
public:
    void begin();
    void update(); // call from loop()

    void onNextPress(ButtonCallback cb);
    void onPrevPress(ButtonCallback cb);
    void onRotaryClick(ButtonCallback cb); // short press
    void onRotaryTurn(EncoderCallback cb);

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

    Button _btnNext;
    Button _btnPrev;
    Button _btnRotSw;

    void initButton(Button &btn, uint8_t pin);
    void updateButton(Button &btn);

    // Rotary via interrupts
    static void IRAM_ATTR isrRotary();
    static volatile int _encoderDelta;
    EncoderCallback _encoderCb = nullptr;
};
