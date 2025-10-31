#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <Arduino.h>
#include "DeviceManager.h"

class AutoMode
{
public:
    AutoMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
};

#endif // AUTOMODE_H