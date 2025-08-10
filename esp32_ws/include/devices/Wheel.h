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
class Wheel : public Device {
public:
    Wheel(int pin1, int pin2, int pin3, int pin4, int buttonPin, const String& id, const String& name,
          float maxSpeed = 1000.0, float acceleration = 500.0,
          bool buttonPullUp = true, unsigned long buttonDebounceMs = 50,
          Button::ButtonType buttonType = Button::ButtonType::NormalOpen);
    void loop() override;
    bool control(const String& action, JsonObject* payload = nullptr) override;
    String getState() override;
    String getId() const override { return _id; }
    String getType() const override { return _type; }
    String getName() const override { return _name; }
    void spin(long steps);
private:
    Stepper* _stepper;
    Button* _sensor;
    String _id;
    String _name;
    String _type = "MARBLE_WHEEL";
};

#endif // WHEEL_H
