#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include "DeviceManager.h"

class ManualMode
{
public:
    ManualMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
};

#endif // MANUALMODE_H