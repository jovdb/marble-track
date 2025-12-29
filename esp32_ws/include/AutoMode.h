#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <Arduino.h>
#include "DeviceManager.h"

// Forward declarations
class Button;

namespace devices
{
    class Buzzer;
    class Wheel;
    class Lift;
    class Led;
}

class AutoMode
{
public:
    AutoMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
    
    // Device references
    devices::Buzzer *_buzzer;
    
    devices::Wheel *_wheel;
    Button *_wheelNextBtn;
    devices::Led *_wheelBtnLed;
    
    devices::Wheel *_splitter;
    Button *_splitterNextBtn;
    devices::Led *_splitterBtnLed;

    devices::Lift *_lift;
};

#endif // AUTOMODE_H
