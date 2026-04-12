#include "AppController.h"

AppController *AppController::_instance = nullptr;

// ── Static callback trampolines ───────────────────────────
void AppController::onNext()
{
    if (_instance)
        _instance->nextTrack();
}
void AppController::onPrev()
{
    if (_instance)
        _instance->prevTrack();
}
void AppController::onPlayPause()
{
    Serial.println(F("[App] onPlayPause triggered"));
    if (_instance)
        _instance->togglePlayPause();
}
void AppController::onLongPress()
{
    if (_instance)
        _instance->handleLongPress();
}
void AppController::onVolume(int d)
{
    if (_instance)
        _instance->changeVolume(d);
}

void AppController::begin()
{
    _instance = this;

    Serial.begin(115200);
    Serial.println(F("\n=== MP3 Player ==="));

    _ui.begin();

    if (!_sd.begin())
        Serial.println(F("[App] No MP3 files found!"));

    _audio.begin();

    // Init Bluetooth in lazy mode (sink starts only when BT is used).
    _bt.init();

    _input.begin();
    _input.onNextPress(onNext);
    _input.onPrevPress(onPrev);
    _input.onRotaryClick(onPlayPause);
    _input.onRotaryLongPress(onLongPress);
    _input.onRotaryTurn(onVolume);

    _currentTrack = 0;
    _started = false;

    startAudioTask();

    Serial.printf("[App] Ready. %d tracks indexed.\n", _sd.getTrackCount());
}

void AppController::update()
{
    _input.update();
    _bt.update();

    // Drive UI view transitions based on BT state changes
    _ui.notifyBtState(_bt);

    // BT menu open: render it, suppress MP3 UI
    if (_ui.isBtMenuOpen())
    {
        _ui.renderBluetooth(_bt);
        return;
    }

    // ── BT / MP3 coexistence ───────────────────────────────
    if (_bt.isConnected())
    {
        if (_audio.getState() == PlayState::PLAYING)
        {
            _audio.pause();
            _pausedForBt = true;
        }
    }
    else if (_pausedForBt)
    {
        if (_audio.getState() == PlayState::PAUSED)
            _audio.resume();
        _pausedForBt = false;
    }

    if (_audio.trackFinished() && _started && !_bt.isConnected())
        nextTrack();

    // Normal player display
    _ui.update(
        _sd.getTrackName(_currentTrack),
        _audio.getState(),
        _audio.getProgress(),
        _audio.getPositionSec(),
        _audio.getDurationSec(),
        _audio.getVolume(),
        _currentTrack,
        _sd.getTrackCount());
}

// ── Control methods ───────────────────────────────────────
void AppController::nextTrack()
{
    uint16_t total = _sd.getTrackCount();
    if (total == 0)
        return;
    _currentTrack = (_currentTrack + 1) % total;
    if (_started)
    {
        _audio.play(_sd.getTrackPath(_currentTrack));
    }
}

void AppController::prevTrack()
{
    uint16_t total = _sd.getTrackCount();
    if (total == 0)
        return;
    _currentTrack = (_currentTrack - 1 + total) % total;
    if (_started)
    {
        _audio.play(_sd.getTrackPath(_currentTrack));
    }
}

void AppController::togglePlayPause()
{
    Serial.println(F("[App] togglePlayPause() entered"));

    // Short press while BT menu open → select menu item
    if (_ui.isBtMenuOpen())
    {
        Serial.println(F("[App] BT menu open: routing click to BT UI action"));
        UIAction action = _ui.onBtClick(_bt);
        handleBtAction(action);
        return;
    }

    uint16_t total = _sd.getTrackCount();
    Serial.printf("[App] Track count before refresh: %u\n", (unsigned)total);
    if (total == 0)
    {
        Serial.println(F("[App] Track count is 0, retrying SD index..."));
        if (!_sd.begin())
        {
            Serial.println(F("[App] ERROR: SD re-index failed"));
            return;
        }
        total = _sd.getTrackCount();
        Serial.printf("[App] Track count after refresh: %u\n", (unsigned)total);
        if (total == 0)
        {
            Serial.println(F("[App] ERROR: No tracks available"));
            return;
        }
    }

    if (_currentTrack >= total)
    {
        _currentTrack = 0;
        Serial.println(F("[App] Current track index reset to 0"));
    }

    const char *path = _sd.getTrackPath(_currentTrack);
    if (!path)
    {
        Serial.printf("[App] ERROR: getTrackPath returned null for index %u\n", (unsigned)_currentTrack);
        return;
    }
    Serial.printf("[App] Track path: %s\n", path);

    PlayState st = _audio.getState();
    Serial.printf("[App] Audio state before action: %d\n", (int)st);

    // MP3 should own I2S when user requests play/resume.
    if (st == PlayState::STOPPED || st == PlayState::PAUSED)
    {
        Serial.println(F("[App] Releasing BT I2S for MP3"));
        _bt.releaseI2S();
        delay(100);
    }

    switch (st)
    {
    case PlayState::STOPPED:
        _started = true;
        Serial.println(F("[App] Transition STOPPED -> PLAYING (play)"));
        _audio.play(path);
        break;
    case PlayState::PLAYING:
        Serial.println(F("[App] Transition PLAYING -> PAUSED"));
        _audio.pause();
        break;
    case PlayState::PAUSED:
        Serial.println(F("[App] Transition PAUSED -> PLAYING (resume)"));
        _audio.resume();
        break;
    }

    Serial.printf("[App] Audio state after action: %d\n", (int)_audio.getState());
}

void AppController::handleLongPress()
{
    if (_bt.getState() == BT_SCANNING)
        _bt.stopScan();
    _ui.onBtLongPress(_bt);
}

void AppController::changeVolume(int dir)
{
    // Rotary scroll while BT menu open → navigate menu
    if (_ui.isBtMenuOpen())
    {
        _ui.onBtRotate(dir, _bt);
        return;
    }
    int vol = _audio.getVolume() + dir * VOLUME_STEP;
    _audio.setVolume(vol);
}

void AppController::handleBtAction(const UIAction &action)
{
    switch (action.type)
    {
    case UIActionType::NONE:
        break;
    case UIActionType::START_SCAN:
    case UIActionType::RESCAN:
        _bt.startScan(BT_SCAN_TIMEOUT_MS);
        break;
    case UIActionType::RETRY:
        _bt.connectPaired();
        break;
    case UIActionType::CONNECT_FOUND:
        if (action.index >= 0 && action.index < (int)_bt.foundCount())
        {
            const BTDevice &d = _bt.foundDevice(action.index);
            _bt.connectToDevice(d.name, d.mac);
        }
        break;
    }
}

// ── FreeRTOS audio task ───────────────────────────────────
// Runs on core 1, continuously pumps the MP3 decoder
// so audio never stutters regardless of UI/input load on core 0
void AppController::audioTask(void *param)
{
    AudioManager *audio = static_cast<AudioManager *>(param);
    for (;;)
    {
        audio->loop();
        // Yield ~125µs — enough for 44.1kHz I2S feeding
        vTaskDelay(1);
    }
}

void AppController::startAudioTask()
{
    xTaskCreatePinnedToCore(
        audioTask,
        "audio",
        AUDIO_TASK_STACK,
        &_audio,
        AUDIO_TASK_PRIO,
        nullptr,
        AUDIO_TASK_CORE);
}
