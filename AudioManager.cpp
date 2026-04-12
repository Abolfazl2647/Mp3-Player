#include "AudioManager.h"
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "driver/i2s.h"

// ESP8266Audio objects — kept in .cpp to avoid header pollution
static AudioFileSourceSD *_source = nullptr;
static AudioGeneratorMP3 *_mp3 = nullptr;
static AudioOutputI2S *_out = nullptr;

void AudioManager::begin()
{
    if (!reinitI2S())
    {
        return;
    }

    _mp3 = new AudioGeneratorMP3();
    if (!_mp3)
    {
        Serial.println(F("[Audio] ERROR: AudioGeneratorMP3 allocation failed"));
        return;
    }

    _state = PlayState::STOPPED;
#if DEBUG_SERIAL
    Serial.println(F("[Audio] I2S initialized OK"));
#endif
}

bool AudioManager::reinitI2S()
{
    if (_out)
    {
        _out->stop();
        delete _out;
        _out = nullptr;
    }

    // If BT previously owned I2S, clear stale driver state first.
    i2s_driver_uninstall(I2S_NUM_0);
    delay(50);

    _out = new AudioOutputI2S();
    if (!_out)
    {
        Serial.println(F("[Audio] ERROR: AudioOutputI2S allocation failed"));
        return false;
    }

    _out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
    _out->begin();
    _out->SetRate(44100);
    _out->SetGain(((float)_volume) / (float)VOLUME_MAX);

#if DEBUG_SERIAL
    Serial.println(F("[Audio] I2S reinitialized"));
#endif
    return true;
}

void AudioManager::play(const char *path)
{
#if DEBUG_SERIAL
    Serial.printf("[Audio] play() called: %s\n", path ? path : "(null)");
#endif

    if (!path || path[0] == '\0')
    {
        Serial.println(F("[Audio] ERROR: Empty path"));
        return;
    }

    stop(); // clean up any previous playback

    bool exists = SD.exists(path);
#if DEBUG_SERIAL
    Serial.printf("[Audio] File exists: %d\n", exists ? 1 : 0);
#endif
    if (!exists)
    {
        Serial.printf("[Audio] ERROR: File not found: %s\n", path);
        return;
    }

    if (!_out || !_mp3)
    {
        Serial.println(F("[Audio] ERROR: Audio pipeline not initialized"));
        return;
    }

    File f = SD.open(path);
    if (f)
    {
        _fileSize = f.size();
#if DEBUG_SERIAL
        Serial.printf("[Audio] File size: %u bytes\n", (unsigned)_fileSize);
#endif
        f.close();
    }
    else
    {
        _fileSize = 0;
        Serial.printf("[Audio] ERROR: SD.open failed: %s\n", path);
        return;
    }

    strncpy(_currentPath, path, sizeof(_currentPath) - 1);
    _currentPath[sizeof(_currentPath) - 1] = '\0';

    _source = new AudioFileSourceSD(path);
    if (!_source)
    {
        Serial.println(F("[Audio] ERROR: Failed to create AudioFileSourceSD"));
        return;
    }

    // Keep I2S channel persistent; avoid re-initializing on each track change.
    _out->SetGain(((float)_volume) / (float)VOLUME_MAX);

    bool beginOk = _mp3->begin(_source, _out);
#if DEBUG_SERIAL
    Serial.printf("[Audio] MP3 begin result: %s\n", beginOk ? "true" : "false");
#endif

    if (!beginOk)
    {
#if DEBUG_SERIAL
        Serial.println(F("[Audio] MP3 begin failed, retry after I2S reinit"));
#endif
        reinitI2S();
        beginOk = _mp3->begin(_source, _out);
#if DEBUG_SERIAL
        Serial.printf("[Audio] MP3 begin retry result: %s\n", beginOk ? "true" : "false");
#endif
    }

    if (beginOk)
    {
        _state = PlayState::PLAYING;
        _finished = false;
        _playStartMs = millis();
        _pausedElapsed = 0;
#if DEBUG_SERIAL
        Serial.printf("[Audio] PLAYBACK STARTED: %s\n", path);
#endif
    }
    else
    {
        Serial.println(F("[Audio] ERROR: MP3 begin failed"));
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
    if (_out)
    {
        _out->stop();
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
#if DEBUG_SERIAL
            Serial.println(F("[Audio] Track finished"));
#endif
        }
    }
}
