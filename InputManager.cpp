#include "InputManager.h"

void InputManager::initButton(Button &btn, uint8_t pin)
{
    btn.pin = pin;
    pinMode(pin, INPUT_PULLUP);
    btn.lastStable = HIGH; // Assume released (active-LOW buttons)
    btn.lastReading = HIGH;
    btn.lastChangeMs = millis();
    btn.callback = nullptr;
}

void InputManager::begin()
{
    // Initialize 5-way module buttons
    initButton(_btn5wayU, PIN_5WAY_U);
    initButton(_btn5wayD, PIN_5WAY_D);
    initButton(_btn5wayL, PIN_5WAY_L);
    initButton(_btn5wayR, PIN_5WAY_R);
    initButton(_btn5wayC, PIN_5WAY_C);

    // Initialize volume buttons
    initButton(_btnVolUp, PIN_VOL_UP);
    initButton(_btnVolDown, PIN_VOL_DOWN);

#if DEBUG_SERIAL
    Serial.println(F("[Input] 5-way + volume buttons initialized"));
#endif
}

void InputManager::updateButton(Button &btn)
{
    bool reading = digitalRead(btn.pin);
    if (reading != btn.lastReading)
    {
        btn.lastChangeMs = millis();
    }
    btn.lastReading = reading;

    // Debounce: fire callback on transition from HIGH→LOW (button press)
    if ((millis() - btn.lastChangeMs) > DEBOUNCE_MS)
    {
        if (!reading && btn.lastStable)
        {
            // Button just pressed (transition from released to pressed)
            if (btn.callback)
                btn.callback();
        }
        btn.lastStable = reading;
    }
}

void InputManager::updateButtonWithLongPress(Button &btn, LongPressState &longPress)
{
    bool reading = digitalRead(btn.pin);
    if (reading != btn.lastReading)
    {
        btn.lastChangeMs = millis();
    }
    btn.lastReading = reading;

    unsigned long debounceWait = millis() - btn.lastChangeMs;

    if (debounceWait > DEBOUNCE_MS)
    {
        if (!reading && btn.lastStable)
        {
            // Button just pressed
            longPress.pressStartMs = millis();
            longPress.isPressed = true;
        }
        else if (reading && !btn.lastStable)
        {
            // Button just released
            longPress.isPressed = false;

            // Fire callback only if this was NOT a long press
            unsigned long pressDuration = millis() - longPress.pressStartMs;
            if (pressDuration < LONG_PRESS_MS && btn.callback)
            {
                btn.callback();
            }
        }
        btn.lastStable = reading;
    }
}

void InputManager::update()
{
    // Update 5-way navigation buttons (except C, which handles long-press)
    updateButton(_btn5wayU);
    updateButton(_btn5wayD);
    updateButton(_btn5wayL);
    updateButton(_btn5wayR);

    // Update C button with long-press detection
    updateButtonWithLongPress(_btn5wayC, _selectLongPress);

    // Update volume buttons
    updateButton(_btnVolUp);
    updateButton(_btnVolDown);
}

bool InputManager::isSelectLongPressed() const
{
    if (!_selectLongPress.isPressed)
        return false;

    unsigned long pressDuration = millis() - _selectLongPress.pressStartMs;
    return pressDuration >= LONG_PRESS_MS;
}

// Menu navigation callbacks
void InputManager::onMenuUp(ButtonCallback cb) { _btn5wayU.callback = cb; }
void InputManager::onMenuDown(ButtonCallback cb) { _btn5wayD.callback = cb; }
void InputManager::onMenuSelect(ButtonCallback cb) { _btn5wayC.callback = cb; }

// Playback control callbacks
void InputManager::onPrevTrack(ButtonCallback cb) { _btn5wayL.callback = cb; }
void InputManager::onNextTrack(ButtonCallback cb) { _btn5wayR.callback = cb; }
void InputManager::onPlayPause(ButtonCallback cb) { _btn5wayC.callback = cb; }

// Volume callbacks
void InputManager::onVolumeUp(ButtonCallback cb) { _btnVolUp.callback = cb; }
void InputManager::onVolumeDown(ButtonCallback cb) { _btnVolDown.callback = cb; }
