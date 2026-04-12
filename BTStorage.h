#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

struct BTSavedDevice
{
    String name;
    String mac;
    bool valid = false;
};

class BTStorage
{
public:
    void begin();
    void end();

    uint8_t loadDevices(BTSavedDevice *outDevices, uint8_t maxCount);
    bool saveDevice(const String &name, const String &mac);
    bool removeDevice(uint8_t index);

    int8_t getLastIndex();
    void setLastIndex(int8_t index);

private:
    Preferences _prefs;
    bool _opened = false;

    String keyName(uint8_t index) const;
    String keyMac(uint8_t index) const;
};
