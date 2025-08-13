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
    enum class wheelState
    {
        CALIBRATING,
        IDLE,
        MOVING
    };
    Wheel(int stepPin1, int dirPin, int buttonPin, const String &id, const String &name);
    String getState() override;
    void loop() override;
    bool control(const String &action, JsonObject *payload = nullptr) override;
    void move(long steps);
    void calibrate();

private:
    Stepper *_stepper;
    Button *_sensor;
    wheelState _state;
};

#endif // WHEEL_H
