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
        MOVING,
        RESET
    };
    Wheel(const String &id);
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

private:
    String stateToString(wheelState state) const;
    Stepper *_stepper;
    Button *_sensor;
    wheelState _state;
    /* -1 = CCW, 1 = CW */
    int _direction = -1;
    std::vector<long> _breakPoints;
    long _lastZeroPosition = 0;
    /** Average of the last X revolutions? */
    long _stepsInLastRevolution = 0;
    long _stepsPerRevolution = 0;
};

#endif // WHEEL_H
