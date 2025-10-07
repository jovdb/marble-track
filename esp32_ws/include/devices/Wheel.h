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
    Wheel(const String &id);
    String getState() override;
    void loop() override;
    bool control(const String &action, JsonObject *payload = nullptr) override;
    void move(long steps);
    void calibrate();

private:
    Stepper *_stepper;
    Button *_sensor;
    wheelState _state;
    /* -1 = CCW, 1 = CW */
    byte _direction = -1;
};

#endif // WHEEL_H
