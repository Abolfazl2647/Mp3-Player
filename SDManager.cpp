#include "SDManager.h"
#include <SD.h>
#include <SPI.h>
#include <cstring>

bool SDManager::begin()
{
    if (!SD.begin(PIN_SD_CS))
    {
        Serial.println(F("[SD] Mount failed"));
        return false;
    }
    Serial.println(F("[SD] Mounted OK"));
    _count = 0;
    scanDirectory(MP3_ROOT_DIR);
    // Sort alphabetically for predictable ordering (insertion sort)
    for (uint16_t i = 1; i < _count; i++)
    {
        for (uint16_t j = i; j > 0 && strcasecmp(_paths[j - 1], _paths[j]) > 0; j--)
        {
            char tmp[64];
            memcpy(tmp, _paths[j], 64);
            memcpy(_paths[j], _paths[j - 1], 64);
            memcpy(_paths[j - 1], tmp, 64);
        }
    }
    Serial.printf("[SD] Found %d MP3 files\n", _count);
    return _count > 0;
}

void SDManager::scanDirectory(const char *dirPath)
{
    File root = SD.open(dirPath);
    if (!root || !root.isDirectory())
        return;

    File entry;
    while ((entry = root.openNextFile()) && _count < MAX_TRACKS)
    {
        if (entry.isDirectory())
        {
            // Recurse into subdirectories
            scanDirectory(entry.path());
        }
        else
        {
            const char *name = entry.name();
            size_t len = strlen(name);
            // Accept .mp3 files only
            if (len > 4 &&
                (strcasecmp(name + len - 4, ".mp3") == 0))
            {
                strncpy(_paths[_count], entry.path(), sizeof(_paths[0]) - 1);
                _paths[_count][sizeof(_paths[0]) - 1] = '\0';
                _count++;
            }
        }
        entry.close();
    }
    root.close();
}

uint16_t SDManager::getTrackCount() const
{
    return _count;
}

const char *SDManager::getTrackPath(uint16_t index) const
{
    if (index >= _count)
        return nullptr;
    return _paths[index];
}

const char *SDManager::getTrackName(uint16_t index) const
{
    if (index >= _count)
        return "---";
    // Return filename without leading path
    const char *p = strrchr(_paths[index], '/');
    return p ? (p + 1) : _paths[index];
}
