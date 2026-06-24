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
#if DEBUG_SERIAL
    Serial.println(F("[App] onPlayPause triggered"));
#endif
    if (_instance)
        _instance->togglePlayPause();
}
void AppController::onVolume(int d)
{
    if (_instance)
        _instance->changeVolume(d);
}

void AppController::begin()
{
    _instance = this;

#if DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println(F("\n=== MP3 Player ==="));
#endif

    _ui.begin();

    if (!_sd.begin())
        Serial.println(F("[App] No MP3 files found!"));

    _audio.begin();

    _input.begin();
    _input.onNextPress(onNext);
    _input.onPrevPress(onPrev);
    _input.onRotaryClick(onPlayPause);
    _input.onRotaryTurn(onVolume);

    _currentTrack = 0;
    _started = false;

    startAudioTask();

#if DEBUG_SERIAL
    Serial.printf("[App] Ready. %d tracks indexed.\n", _sd.getTrackCount());
#endif
}

void AppController::update()
{
    _input.update();

    if (_audio.trackFinished() && _started)
        nextTrack();

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
#if DEBUG_SERIAL
    Serial.println(F("[App] togglePlayPause() entered"));
#endif

    uint16_t total = _sd.getTrackCount();
#if DEBUG_SERIAL
    Serial.printf("[App] Track count before refresh: %u\n", (unsigned)total);
#endif
    if (total == 0)
    {
#if DEBUG_SERIAL
        Serial.println(F("[App] Track count is 0, retrying SD index..."));
#endif
        if (!_sd.begin())
        {
            Serial.println(F("[App] ERROR: SD re-index failed"));
            return;
        }
        total = _sd.getTrackCount();
#if DEBUG_SERIAL
        Serial.printf("[App] Track count after refresh: %u\n", (unsigned)total);
#endif
        if (total == 0)
        {
            Serial.println(F("[App] ERROR: No tracks available"));
            return;
        }
    }

    if (_currentTrack >= total)
    {
        _currentTrack = 0;
#if DEBUG_SERIAL
        Serial.println(F("[App] Current track index reset to 0"));
#endif
    }

    const char *path = _sd.getTrackPath(_currentTrack);
    if (!path)
    {
        Serial.printf("[App] ERROR: getTrackPath returned null for index %u\n", (unsigned)_currentTrack);
        return;
    }
#if DEBUG_SERIAL
    Serial.printf("[App] Track path: %s\n", path);
#endif

    PlayState st = _audio.getState();
#if DEBUG_SERIAL
    Serial.printf("[App] Audio state before action: %d\n", (int)st);
#endif

    switch (st)
    {
    case PlayState::STOPPED:
        _started = true;
#if DEBUG_SERIAL
        Serial.println(F("[App] Transition STOPPED -> PLAYING (play)"));
#endif
        _audio.play(path);
        break;
    case PlayState::PLAYING:
#if DEBUG_SERIAL
        Serial.println(F("[App] Transition PLAYING -> PAUSED"));
#endif
        _audio.pause();
        break;
    case PlayState::PAUSED:
#if DEBUG_SERIAL
        Serial.println(F("[App] Transition PAUSED -> PLAYING (resume)"));
#endif
        _audio.resume();
        break;
    }

#if DEBUG_SERIAL
    Serial.printf("[App] Audio state after action: %d\n", (int)_audio.getState());
#endif
}

void AppController::changeVolume(int dir)
{
    int vol = _audio.getVolume() + dir * VOLUME_STEP;
    _audio.setVolume(vol);
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
