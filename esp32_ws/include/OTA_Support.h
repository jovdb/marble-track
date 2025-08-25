

#ifndef OTA_SUPPORT_H
#define OTA_SUPPORT_H

#include <Arduino.h>

class OTAService
{
public:
    OTAService();
    void setup(const char *hostname = "marble-track.local");
    void loop();

private:
    const char *_hostname = nullptr;
};

#endif // OTA_SUPPORT_H
