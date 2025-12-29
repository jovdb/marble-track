#ifndef NETWORKSETTINGS_H
#define NETWORKSETTINGS_H

#include <Arduino.h>

// Network settings structure
struct NetworkSettings {
    String ssid;
    String password;
    
    NetworkSettings() : ssid(""), password("") {}
    NetworkSettings(const String& s, const String& p) : ssid(s), password(p) {}
    
    bool isValid() const { return !ssid.isEmpty(); }
};

#endif // NETWORKSETTINGS_H
