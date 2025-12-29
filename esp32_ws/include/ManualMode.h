#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include <functional>
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
    devices::Button *_wheelNextBtn;
    devices::Wheel *_wheel;
    devices::Buzzer *_buzzer;
    devices::Led *_wheelBtnLed;
    devices::Button *_splitterNextBtn;
    devices::Led *_splitterBtnLed;
    devices::Wheel *_splitter;
    std::function<void()> _wheelNextBtnUnsubscribe;
};

#endif // MANUALMODE_H
