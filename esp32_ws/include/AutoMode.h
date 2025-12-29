#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <Arduino.h>
#include "DeviceManager.h"

// Forward declarations
class Button;

namespace composition
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
    composition::Buzzer *_buzzer;
    
    composition::Wheel *_wheel;
    Button *_wheelNextBtn;
    composition::Led *_wheelBtnLed;
    
    composition::Wheel *_splitter;
    Button *_splitterNextBtn;
    composition::Led *_splitterBtnLed;

    composition::Lift *_lift;
};

#endif // AUTOMODE_H