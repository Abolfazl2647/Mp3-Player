#include "BTStorage.h"

void BTStorage::begin()
{
    if (!_opened)
    {
        _opened = _prefs.begin("btstore", false);
    }
}

void BTStorage::end()
{
    if (_opened)
    {
        _prefs.end();
        _opened = false;
    }
}

String BTStorage::keyName(uint8_t index) const
{
    return String("n") + index;
}

String BTStorage::keyMac(uint8_t index) const
{
    return String("m") + index;
}

uint8_t BTStorage::loadDevices(BTSavedDevice *outDevices, uint8_t maxCount)
{
    if (!_opened || !outDevices)
    {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < BT_MAX_SAVED_DEVICES && i < maxCount; i++)
    {
        String name = _prefs.getString(keyName(i).c_str(), "");
        String mac = _prefs.getString(keyMac(i).c_str(), "");

        if (name.length() > 0 && mac.length() > 0)
        {
            outDevices[count].name = name;
            outDevices[count].mac = mac;
            outDevices[count].valid = true;
            count++;
        }
    }

    return count;
}

bool BTStorage::saveDevice(const String &name, const String &mac)
{
    if (!_opened || name.isEmpty() || mac.isEmpty())
    {
        return false;
    }

    BTSavedDevice devices[BT_MAX_SAVED_DEVICES];
    uint8_t count = loadDevices(devices, BT_MAX_SAVED_DEVICES);

    for (uint8_t i = 0; i < count; i++)
    {
        if (devices[i].mac == mac)
        {
            _prefs.putString(keyName(i).c_str(), name);
            _prefs.putString(keyMac(i).c_str(), mac);
            setLastIndex(i);
            return true;
        }
    }

    if (count < BT_MAX_SAVED_DEVICES)
    {
        _prefs.putString(keyName(count).c_str(), name);
        _prefs.putString(keyMac(count).c_str(), mac);
        setLastIndex(count);
        return true;
    }

    for (uint8_t i = 1; i < BT_MAX_SAVED_DEVICES; i++)
    {
        String prevName = _prefs.getString(keyName(i).c_str(), "");
        String prevMac = _prefs.getString(keyMac(i).c_str(), "");
        _prefs.putString(keyName(i - 1).c_str(), prevName);
        _prefs.putString(keyMac(i - 1).c_str(), prevMac);
    }

    uint8_t slot = BT_MAX_SAVED_DEVICES - 1;
    _prefs.putString(keyName(slot).c_str(), name);
    _prefs.putString(keyMac(slot).c_str(), mac);
    setLastIndex(slot);
    return true;
}

bool BTStorage::removeDevice(uint8_t index)
{
    if (!_opened || index >= BT_MAX_SAVED_DEVICES)
    {
        return false;
    }

    for (uint8_t i = index + 1; i < BT_MAX_SAVED_DEVICES; i++)
    {
        String n = _prefs.getString(keyName(i).c_str(), "");
        String m = _prefs.getString(keyMac(i).c_str(), "");
        _prefs.putString(keyName(i - 1).c_str(), n);
        _prefs.putString(keyMac(i - 1).c_str(), m);
    }

    uint8_t last = BT_MAX_SAVED_DEVICES - 1;
    _prefs.remove(keyName(last).c_str());
    _prefs.remove(keyMac(last).c_str());

    int8_t lastIdx = getLastIndex();
    if (lastIdx == (int8_t)index)
    {
        setLastIndex(-1);
    }
    else if (lastIdx > (int8_t)index)
    {
        setLastIndex(lastIdx - 1);
    }

    return true;
}

int8_t BTStorage::getLastIndex()
{
    if (!_opened)
    {
        return -1;
    }
    return _prefs.getChar("last", -1);
}

void BTStorage::setLastIndex(int8_t index)
{
    if (_opened)
    {
        _prefs.putChar("last", index);
    }
}
