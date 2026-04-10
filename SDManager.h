#pragma once
#include <Arduino.h>

// Forward‐declare the maximum number of tracks from config
#include "Config.h"

class SDManager
{
public:
    bool begin();
    uint16_t getTrackCount() const;
    const char *getTrackPath(uint16_t index) const;
    const char *getTrackName(uint16_t index) const;

private:
    void scanDirectory(const char *dir);
    char _paths[MAX_TRACKS][64]; // full path (e.g. "/folder/song.mp3")
    uint16_t _count = 0;
};
