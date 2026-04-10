#include "AudioManager.h"
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// ESP8266Audio objects — kept in .cpp to avoid header pollution
static AudioFileSourceSD *_source = nullptr;
static AudioGeneratorMP3 *_mp3 = nullptr;
static AudioOutputI2S *_out = nullptr;

void AudioManager::begin()
{
    _out = new AudioOutputI2S();
    _out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
    _out->SetGain(((float)_volume) / (float)VOLUME_MAX);
    _mp3 = new AudioGeneratorMP3();
    _state = PlayState::STOPPED;
}

void AudioManager::play(const char *path)
{
    stop(); // clean up any previous playback

    File f = SD.open(path);
    if (f)
    {
        _fileSize = f.size();
        f.close();
    }
    else
    {
        _fileSize = 0;
    }

    strncpy(_currentPath, path, sizeof(_currentPath) - 1);
    _currentPath[sizeof(_currentPath) - 1] = '\0';

    _source = new AudioFileSourceSD(path);
    _out->SetGain(((float)_volume) / (float)VOLUME_MAX);

    if (_mp3->begin(_source, _out))
    {
        _state = PlayState::PLAYING;
        _finished = false;
        _playStartMs = millis();
        _pausedElapsed = 0;
        Serial.printf("[Audio] Playing: %s\n", path);
    }
    else
    {
        Serial.println(F("[Audio] Failed to begin MP3"));
        delete _source;
        _source = nullptr;
        _state = PlayState::STOPPED;
    }
}

void AudioManager::stop()
{
    if (_mp3 && _mp3->isRunning())
    {
        _mp3->stop();
    }
    if (_source)
    {
        delete _source;
        _source = nullptr;
    }
    _state = PlayState::STOPPED;
    _finished = false;
    _playStartMs = 0;
    _pausedElapsed = 0;
}

void AudioManager::pause()
{
    if (_state == PlayState::PLAYING)
    {
        _pausedElapsed = millis() - _playStartMs;
        // Save file position before stopping
        if (_source)
            _pauseFilePos = _source->getPos();
        // Stop decoder + I2S to silence the DAC immediately
        if (_mp3 && _mp3->isRunning())
            _mp3->stop();
        if (_source)
        {
            delete _source;
            _source = nullptr;
        }
        _state = PlayState::PAUSED;
    }
}

void AudioManager::resume()
{
    if (_state == PlayState::PAUSED)
    {
        _playStartMs = millis() - _pausedElapsed;
        // Re-open file and seek to saved position
        _source = new AudioFileSourceSD(_currentPath);
        _source->seek(_pauseFilePos, SEEK_SET);
        _out->SetGain(((float)_volume) / (float)VOLUME_MAX);
        if (_mp3->begin(_source, _out))
        {
            _state = PlayState::PLAYING;
        }
        else
        {
            Serial.println(F("[Audio] Resume failed"));
            delete _source;
            _source = nullptr;
            _state = PlayState::STOPPED;
        }
    }
}

void AudioManager::setVolume(int vol)
{
    _volume = constrain(vol, VOLUME_MIN, VOLUME_MAX);
    if (_out)
    {
        _out->SetGain(((float)_volume) / (float)VOLUME_MAX);
    }
}

int AudioManager::getVolume() const
{
    return _volume;
}

PlayState AudioManager::getState() const
{
    return _state;
}

bool AudioManager::isPlaying() const
{
    return _state == PlayState::PLAYING;
}

float AudioManager::getProgress() const
{
    if (_fileSize == 0)
        return 0.0f;
    if (_source)
        return (float)_source->getPos() / (float)_fileSize;
    // When paused, source is deleted — use saved position
    if (_state == PlayState::PAUSED)
        return (float)_pauseFilePos / (float)_fileSize;
    return 0.0f;
}

uint32_t AudioManager::getPositionSec() const
{
    if (_state == PlayState::STOPPED)
        return 0;
    unsigned long elapsed;
    if (_state == PlayState::PAUSED)
    {
        elapsed = _pausedElapsed;
    }
    else
    {
        elapsed = millis() - _playStartMs;
    }
    return elapsed / 1000;
}

uint32_t AudioManager::getDurationSec() const
{
    // Rough estimate: MP3 ~16 kByte/s at 128kbps
    if (_fileSize == 0)
        return 0;
    return _fileSize / 16000;
}

bool AudioManager::trackFinished()
{
    if (_finished)
    {
        _finished = false;
        return true;
    }
    return false;
}

void AudioManager::loop()
{
    if (_state != PlayState::PLAYING)
        return;
    if (_mp3 && _mp3->isRunning())
    {
        if (!_mp3->loop())
        {
            // Decoder returned false — track finished
            _mp3->stop();
            _state = PlayState::STOPPED;
            _finished = true;
            if (_source)
            {
                delete _source;
                _source = nullptr;
            }
            Serial.println(F("[Audio] Track finished"));
        }
    }
}
