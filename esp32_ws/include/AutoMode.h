#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <Arduino.h>
#include "DeviceManager.h"

// Forward declarations
class Button;
class Buzzer;
class Wheel;
class Led;
class Lift;

class AutoMode
{
public:
    AutoMode(DeviceManager &deviceManager);
    void setup();
    void loop();

private:
    DeviceManager &deviceManager;
    
    // Device references
    Buzzer *_buzzer;
    
    Wheel *_wheel;
    Button *_wheelNextBtn;
    Led *_wheelBtnLed;
    
    Wheel *_splitter;
    Button *_splitterNextBtn;
    Led *_splitterBtnLed;

    Lift *_lift;
};

#endif // AUTOMODE_H