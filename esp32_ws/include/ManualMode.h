#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Wheel.h"
#include "devices/Buzzer.h"
#include "devices/Led.h"

class ManualMode
{
public:
    ManualMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
    Button *_wheelNextBtn;
    Wheel *_wheel;
    Buzzer *_buzzer;
    Led *_wheelBtnLed;
};

#endif // MANUALMODE_H