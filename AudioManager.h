#pragma once
#include <Arduino.h>
#include "Config.h"

enum class PlayState : uint8_t
{
    STOPPED,
    PLAYING,
    PAUSED
};

class AudioManager
{
public:
    void begin();
    void play(const char *path);
    void stop();
    void pause();
    void resume();
    void setVolume(int vol);
    int getVolume() const;

    PlayState getState() const;
    bool isPlaying() const;
    float getProgress() const; // 0.0–1.0
    uint32_t getPositionSec() const;
    uint32_t getDurationSec() const;

    // Called from FreeRTOS task — pumps the decoder
    void loop();

    // Flag: current track finished
    bool trackFinished();

private:
    bool reinitI2S();
    volatile PlayState _state = PlayState::STOPPED; // volatile: accessed from both cores
    int _volume = VOLUME_DEFAULT;
    volatile bool _finished = false;
    uint32_t _fileSize = 0;
    unsigned long _playStartMs = 0;
    unsigned long _pausedElapsed = 0;
    uint32_t _pauseFilePos = 0; // byte offset for resume
    char _currentPath[64];      // remember path for resume
};
