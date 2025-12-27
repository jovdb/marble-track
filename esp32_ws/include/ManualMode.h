#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Wheel.h"
#include "devices/composition/Buzzer.h"
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
    composition::Buzzer *_buzzer;
    Led *_wheelBtnLed;
    Button *_splitterNextBtn;
    Led *_splitterBtnLed;
    Wheel *_splitter;
};

#endif // MANUALMODE_H