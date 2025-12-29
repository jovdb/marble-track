#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include <functional>
#include "DeviceManager.h"
#include "devices/composition/Button.h"
#include "devices/composition/Wheel.h"
#include "devices/composition/Buzzer.h"
#include "devices/composition/Led.h"

class ManualMode
{
public:
    ManualMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
    composition::Button *_wheelNextBtn;
    composition::Wheel *_wheel;
    composition::Buzzer *_buzzer;
    composition::Led *_wheelBtnLed;
    composition::Button *_splitterNextBtn;
    composition::Led *_splitterBtnLed;
    composition::Wheel *_splitter;
    std::function<void()> _wheelNextBtnUnsubscribe;
};

#endif // MANUALMODE_H