#pragma once
#include "AudioManager.h"
#include "SDManager.h"
#include "UIManager.h"
#include "InputManager.h"

enum class ScreenState : uint8_t
{
    SPLASH,
    TRACK_BROWSER,
    NOW_PLAYING
};

class AppController
{
public:
    void begin();
    void update(); // called from loop()

private:
    AudioManager _audio;
    SDManager _sd;
    UIManager _ui;
    InputManager _input;

    // Screen state management
    ScreenState _currentScreen = ScreenState::SPLASH;
    unsigned long _splashStartMs = 0;
    const unsigned long SPLASH_DURATION_MS = 2000; // 2 seconds

    // Menu state
    int _menuCursor = 0;        // Currently highlighted track
    int _menuScrollOffset = 0;  // Top-left track visible
    const int MENU_DISPLAY_LINES = 5; // Tracks shown on OLED

    int16_t _currentTrack = 0;
    bool _started = false;

    // Input callbacks (static trampolines → instance methods)
    static AppController *_instance;

    // Menu navigation callbacks
    static void onMenuUp();
    static void onMenuDown();
    static void onMenuSelect();

    // Playback control callbacks
    static void onPrevTrack();
    static void onNextTrack();
    static void onPlayPause();

    // Volume callbacks
    static void onVolumeUp();
    static void onVolumeDown();

    // Control methods
    void menuUp();
    void menuDown();
    void menuSelect();
    void nextTrack();
    void prevTrack();
    void togglePlayPause();
    void changeVolume(int dir);
    void handleLongPressC(); // Check for long-press C in NOW_PLAYING

    // Screen management
    void updateSplashScreen();
    void updateTrackBrowserScreen();
    void updateNowPlayingScreen();

    // FreeRTOS audio task
    static void audioTask(void *param);
    void startAudioTask();
};
