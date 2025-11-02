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
    enum class WheelState
    {
        CALIBRATING,
        IDLE,
        MOVING,
        RESET,
        ERROR,
    };
    Wheel(const String &id, NotifyClients callback = nullptr);
    void setup() override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    void loop() override;
    bool control(const String &action, JsonObject *payload = nullptr) override;
    bool move(long steps);
    bool calibrate();
    bool reset();
    bool moveToAngle(float angle);
    void notifyStepsPerRevolution(long steps);
    bool nextBreakPoint();
    WheelState wheelState;
    bool getOnError() const { return _onError; }

private:
    String stateToString(WheelState state) const;
    Stepper *_stepper;
    Button *_sensor;
    Button *_btnNext;

    /* -1 = CCW, 1 = CW */
    int _direction = -1;
    std::vector<float> _breakPoints;
    long _lastZeroPosition = 0;
    /** Average of the last X revolutions? */
    long _stepsInLastRevolution = 0;
    long _stepsPerRevolution = 0;
    long _maxStepsPerRevolution = 10000;
    int _currentBreakpointIndex = -1;
    int _targetBreakpointIndex = -1;
    float _targetAngle = -1.0f;
    float _zeroPointDegree = 0.0f;
    bool _onError = false;
};

#endif // WHEEL_H
