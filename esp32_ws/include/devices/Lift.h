#ifndef LIFT_H
#define LIFT_H

#include <Arduino.h>
#include "Button.h"
#include "Stepper.h"
#include "PwmMotor.h"
#include "Device.h"

/**
 * @class Lift
 * @brief Device for lifting mechanism with stepper and button sensor
 */
class Lift : public Device
{
public:
    enum class liftState
    {
        IDLE,
        MOVING
    };
    Lift(const String &id, NotifyClients callback = nullptr);
    void setup() override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    void loop() override;
    bool control(const String &action, JsonObject *payload = nullptr) override;
    bool up();
    bool down();

private:
    String stateToString(liftState state) const;
    Stepper *_stepper;
    Button *_limitSwitch;
    Button *_ballSensor;
    PwmMotor *_gate;
    liftState _state;
    long _minSteps = 0;
    long _maxSteps = 1000;
};

#endif // LIFT_H