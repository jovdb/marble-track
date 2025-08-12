#ifndef WHEEL_H
#define WHEEL_H

#include <Arduino.h>
#include "Button.h"
#include "Stepper.h"
#include "Device.h"

/**
 * @class Wheel
 * @brief Device combining a stepper wheel and a button sensor
 */
class Wheel : public Device
{
public:
    Wheel(int pin1, int pin2, int pin3, int pin4, int buttonPin, const String &id, const String &name);
    String getState() override;
    void loop() override;
    bool control(const String &action, JsonObject *payload = nullptr) override;
    void spin(long steps);

private:
    Stepper *_stepper;
    Button *_sensor;
};

#endif // WHEEL_H
