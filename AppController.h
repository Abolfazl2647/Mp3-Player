#pragma once
#include "AudioManager.h"
#include "SDManager.h"
#include "UIManager.h"
#include "InputManager.h"

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

    int16_t _currentTrack = 0;
    bool _started = false;

    // Input callbacks (static trampolines → instance methods)
    static AppController *_instance;
    static void onNext();
    static void onPrev();
    static void onPlayPause();
    static void onVolume(int dir);

    void nextTrack();
    void prevTrack();
    void togglePlayPause();
    void changeVolume(int dir);

    // FreeRTOS audio task
    static void audioTask(void *param);
    void startAudioTask();
};
