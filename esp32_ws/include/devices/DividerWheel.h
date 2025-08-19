#ifndef DIVIDERWHEEL_H
#define DIVIDERWHEEL_H

#include "devices/Stepper.h"
#include "devices/Button.h"
#include "devices/Device.h"
#include <vector>


class DividerWheel : public Device {
public:
    enum wheelState { IDLE, CALIBRATING };

    DividerWheel(int stepPin1, int stepPin2, int stepPin3, int stepPin4, int buttonPin, const String &id, const String &name);
    ~DividerWheel();

    void setup() override;
    void loop() override;
    Stepper* getStepper();
    Button* getButton();

    void move(long steps);
    void calibrate();
    bool control(const String &action, JsonObject *payload);
    String getState() override;

public:
    enum CalibrationState { NO, YES, FAILED };
private:
    Stepper *_stepper;
    Button *_button;
    wheelState _state = IDLE;
    CalibrationState _calibrationState = NO;
};

#endif // DIVIDERWHEEL_H
