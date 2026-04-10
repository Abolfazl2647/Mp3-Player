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

    Serial.begin(115200);
    Serial.println(F("\n=== MP3 Player ==="));

    _ui.begin();

    if (!_sd.begin())
    {
        Serial.println(F("[App] No MP3 files found!"));
        // UI will show "---" — system idles
    }

    _audio.begin();

    _input.begin();
    _input.onNextPress(onNext);
    _input.onPrevPress(onPrev);
    _input.onRotaryClick(onPlayPause);
    _input.onRotaryTurn(onVolume);

    _currentTrack = 0;
    _started = false;

    // Launch audio decoder on core 1 as high-priority task
    startAudioTask();

    Serial.printf("[App] Ready. %d tracks indexed.\n", _sd.getTrackCount());
}

void AppController::update()
{
    // Poll inputs (runs on core 0 — main loop)
    _input.update();

    // Auto-advance when track finishes (nextTrack already calls play)
    if (_audio.trackFinished() && _started)
    {
        nextTrack();
    }

    // Update display
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
    if (_sd.getTrackCount() == 0)
        return;

    switch (_audio.getState())
    {
    case PlayState::STOPPED:
        _started = true;
        _audio.play(_sd.getTrackPath(_currentTrack));
        break;
    case PlayState::PLAYING:
        _audio.pause();
        break;
    case PlayState::PAUSED:
        _audio.resume();
        break;
    }
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
