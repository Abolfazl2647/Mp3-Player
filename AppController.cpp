#include "AppController.h"

AppController *AppController::_instance = nullptr;

// ── Static callback trampolines ───────────────────────────
void AppController::onMenuUp()
{
    if (_instance)
        _instance->menuUp();
}
void AppController::onMenuDown()
{
    if (_instance)
        _instance->menuDown();
}
void AppController::onMenuSelect()
{
    if (_instance)
        _instance->menuSelect();
}
void AppController::onPrevTrack()
{
    if (_instance)
        _instance->prevTrack();
}
void AppController::onNextTrack()
{
    if (_instance)
        _instance->nextTrack();
}
void AppController::onPlayPause()
{
#if DEBUG_SERIAL
    Serial.println(F("[App] onPlayPause triggered"));
#endif
    if (_instance)
        _instance->togglePlayPause();
}
void AppController::onVolumeUp()
{
    if (_instance)
        _instance->changeVolume(1);
}
void AppController::onVolumeDown()
{
    if (_instance)
        _instance->changeVolume(-1);
}

void AppController::begin()
{
    _instance = this;

#if DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println(F("\n=== MP3 Player (3-Screen UI) ==="));
#endif

    _ui.begin();

    if (!_sd.begin())
        Serial.println(F("[App] No MP3 files found!"));

    _audio.begin();

    _input.begin();

    // Wire up menu navigation
    _input.onMenuUp(onMenuUp);
    _input.onMenuDown(onMenuDown);
    _input.onMenuSelect(onMenuSelect);

    // Wire up playback controls (only active in NOW_PLAYING)
    _input.onPrevTrack(onPrevTrack);
    _input.onNextTrack(onNextTrack);
    _input.onPlayPause(onPlayPause);

    // Wire up volume controls
    _input.onVolumeUp(onVolumeUp);
    _input.onVolumeDown(onVolumeDown);

    _currentTrack = 0;
    _menuCursor = 0;
    _menuScrollOffset = 0;
    _started = false;

    _currentScreen = ScreenState::SPLASH;
    _splashStartMs = millis();

    startAudioTask();

#if DEBUG_SERIAL
    Serial.printf("[App] Ready. %d tracks indexed.\n", _sd.getTrackCount());
#endif
}

void AppController::update()
{
    _input.update();

    // Screen state machine
    switch (_currentScreen)
    {
    case ScreenState::SPLASH:
        updateSplashScreen();
        break;
    case ScreenState::TRACK_BROWSER:
        updateTrackBrowserScreen();
        break;
    case ScreenState::NOW_PLAYING:
        updateNowPlayingScreen();
        break;
    }
}

// ── Screen update methods ─────────────────────────────────
void AppController::updateSplashScreen()
{
    // Show splash for SPLASH_DURATION_MS then auto-transition
    if (millis() - _splashStartMs >= SPLASH_DURATION_MS)
    {
        _currentScreen = ScreenState::TRACK_BROWSER;
#if DEBUG_SERIAL
        Serial.println(F("[App] Splash → Track Browser"));
#endif
    }
    _ui.renderSplash();
}

void AppController::updateTrackBrowserScreen()
{
    // Display track browser
    uint16_t total = _sd.getTrackCount();
    _ui.renderTrackBrowser(_sd, total, _menuCursor, _menuScrollOffset, MENU_DISPLAY_LINES);
}

void AppController::updateNowPlayingScreen()
{
    // Check for long-press C to return to track browser
    handleLongPressC();

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

// ── Menu navigation methods ───────────────────────────────
void AppController::menuUp()
{
    if (_currentScreen != ScreenState::TRACK_BROWSER)
        return;

    uint16_t total = _sd.getTrackCount();
    if (total == 0)
        return;

    _menuCursor = (_menuCursor - 1 + total) % total;

    // Auto-scroll: if cursor moved above visible area
    if (_menuCursor < _menuScrollOffset)
        _menuScrollOffset = _menuCursor;

#if DEBUG_SERIAL
    Serial.printf("[App] Menu cursor: %d\n", _menuCursor);
#endif
}

void AppController::menuDown()
{
    if (_currentScreen != ScreenState::TRACK_BROWSER)
        return;

    uint16_t total = _sd.getTrackCount();
    if (total == 0)
        return;

    _menuCursor = (_menuCursor + 1) % total;

    // Auto-scroll: if cursor moved below visible area
    if (_menuCursor >= _menuScrollOffset + MENU_DISPLAY_LINES)
        _menuScrollOffset = _menuCursor - MENU_DISPLAY_LINES + 1;

#if DEBUG_SERIAL
    Serial.printf("[App] Menu cursor: %d\n", _menuCursor);
#endif
}

void AppController::menuSelect()
{
    if (_currentScreen != ScreenState::TRACK_BROWSER)
        return;

    uint16_t total = _sd.getTrackCount();
    if (total == 0)
        return;

    // Load and play selected track
    _currentTrack = _menuCursor;
    const char *path = _sd.getTrackPath(_currentTrack);
    if (!path)
        return;

    _started = true;
    _audio.play(path);
    _currentScreen = ScreenState::NOW_PLAYING;

#if DEBUG_SERIAL
    Serial.printf("[App] Track selected: %d, now playing\n", _currentTrack);
#endif
}

void AppController::handleLongPressC()
{
    // If C button is long-pressed, return to track browser
    if (_input.isSelectLongPressed())
    {
        _currentScreen = ScreenState::TRACK_BROWSER;
        _menuCursor = _currentTrack; // Keep cursor on current track
        _menuScrollOffset = _currentTrack > 2 ? _currentTrack - 2 : 0;

#if DEBUG_SERIAL
        Serial.println(F("[App] Long-press detected, returning to Track Browser"));
#endif
    }
}

// ── Playback control methods ──────────────────────────────
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
