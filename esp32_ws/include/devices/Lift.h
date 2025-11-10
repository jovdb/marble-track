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
        ERROR,
        INIT,
        LIFT_DOWN_LOADING,
        LIFT_DOWN_LOADED,
        LIFT_DOWN_UNLOADED,
        LIFT_UP_UNLOADING,
        LIFT_UP_UNLOADED,
        LIFT_UP_LOADED,
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
    bool up(float speedRatio = 1.0f);
    bool down(float speedRatio = 1.0f);
    bool init();
    bool loadBall();
    bool unloadBall();
    bool isBallWaiting();

    LiftState liftState;

private:
    bool isLoaded();
    String stateToString(LiftState state) const;
    bool loadBallStart();
    bool loadBallEnd();
    bool unloadBallStart();
    bool unloadBallEnd();
    Stepper *_stepper;
    Button *_limitSwitch;
    Button *_ballSensor;
    PwmMotor *_loader;
    PwmMotor *_unloader;
    long _minSteps = 0;
    long _maxSteps = 1000;
    unsigned long _loadStartTime = 0;
    unsigned long _unloadStartTime = 0;
    unsigned long _unloadEndTime = 0;
    bool _isBallWaiting = false;
    bool _isLoaded = false;
    int _resetStep = 0;
};

#endif // LIFT_H