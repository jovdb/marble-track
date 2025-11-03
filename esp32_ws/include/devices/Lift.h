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
    enum class LiftState
    {
        UNKNOWN,
        IDLE,
        ERROR,
        RESET,
        BALL_WAITING,
        LIFT_LOADED,
        MOVING_UP,
        MOVING_DOWN
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
    bool reset();
    bool gateUp();
    bool gateDown();

private:
    String stateToString(LiftState state) const;
    Stepper *_stepper;
    Button *_limitSwitch;
    Button *_ballSensor;
    PwmMotor *_gate;
    LiftState _state;
    long _minSteps = 0;
    long _maxSteps = 1000;
};

#endif // LIFT_H