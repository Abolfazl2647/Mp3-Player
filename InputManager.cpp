#include "InputManager.h"

volatile int InputManager::_encoderDelta = 0;

// ISR for rotary encoder — Gray-code reading
void IRAM_ATTR InputManager::isrRotary()
{
    static uint8_t prevState = 0;
    uint8_t a = digitalRead(PIN_ROT_CLK);
    uint8_t b = digitalRead(PIN_ROT_DT);
    uint8_t curState = (a << 1) | b;

    // Valid transitions in Gray code sequence
    // 00→01→11→10 = CW,  00→10→11→01 = CCW
    uint8_t combined = (prevState << 2) | curState;
    switch (combined)
    {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
        _encoderDelta++;
        break;
    case 0b0010:
    case 0b1011:
    case 0b1101:
    case 0b0100:
        _encoderDelta--;
        break;
    }
    prevState = curState;
}

void InputManager::initButton(Button &btn, uint8_t pin)
{
    btn.pin = pin;
    pinMode(pin, INPUT_PULLUP);
    btn.lastStable = HIGH; // Assume released (active-LOW buttons)
    btn.lastReading = HIGH;
    btn.lastChangeMs = millis(); // Prevent false trigger at boot
    btn.callback = nullptr;
}

void InputManager::begin()
{
    initButton(_btnNext, PIN_BTN_NEXT);
    initButton(_btnPrev, PIN_BTN_PREV);
    initButton(_btnRotSw, PIN_ROT_SW);

    pinMode(PIN_ROT_CLK, INPUT_PULLUP);
    pinMode(PIN_ROT_DT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ROT_CLK), isrRotary, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ROT_DT), isrRotary, CHANGE);
}

void InputManager::updateButton(Button &btn)
{
    bool reading = digitalRead(btn.pin);
    if (reading != btn.lastReading)
    {
        btn.lastChangeMs = millis();
    }
    btn.lastReading = reading;

    if ((millis() - btn.lastChangeMs) > DEBOUNCE_MS)
    {
        // ── Next / Prev: fire on press-down (original behaviour) ──────
        if (&btn != &_btnRotSw)
        {
            if (!reading && btn.lastStable)
            {
                if (btn.callback)
                    btn.callback();
            }
            btn.lastStable = reading;
            return;
        }

        // ── Rotary switch: fire on release ────────────────────────────
        if (reading && !btn.lastStable)
        {
            if (btn.callback)
                btn.callback();
        }

        btn.lastStable = reading;
    }
}

void InputManager::update()
{
    updateButton(_btnNext);
    updateButton(_btnPrev);
    updateButton(_btnRotSw);

    // Process accumulated encoder ticks
    noInterrupts();
    int delta = _encoderDelta;
    _encoderDelta = 0;
    interrupts();

    // Post-process: 4 raw ticks = 1 detent on most encoders
    // Send callback per detent
    static int accumulator = 0;
    accumulator += delta;
    while (accumulator >= 4)
    {
        accumulator -= 4;
        if (_encoderCb)
            _encoderCb(1);
    }
    while (accumulator <= -4)
    {
        accumulator += 4;
        if (_encoderCb)
            _encoderCb(-1);
    }
}

void InputManager::onNextPress(ButtonCallback cb) { _btnNext.callback = cb; }
void InputManager::onPrevPress(ButtonCallback cb) { _btnPrev.callback = cb; }
void InputManager::onRotaryClick(ButtonCallback cb) { _btnRotSw.callback = cb; }
void InputManager::onRotaryTurn(EncoderCallback cb) { _encoderCb = cb; }
