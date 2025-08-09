#ifndef GATE_WITH_SENSOR_H
#define GATE_WITH_SENSOR_H

#include <Arduino.h>
#include "Servo.h"
#include "Button.h"
#include "Buzzer.h"
#include "Device.h"

/**
 * @class GateWithSensor
 * @brief Device combining a servo gate and a button sensor
 */
class GateWithSensor : public Device {
public:
    GateWithSensor(int servoPin, int servoPwmChannel, int buttonPin, int buzzerPin, const String& id, const String& name,
                   int initialAngle = 90, bool buttonPullUp = true, unsigned long buttonDebounceMs = 50,
                   Button::ButtonType buttonType = Button::ButtonType::NormalOpen);
    void setup();
    void loop();
    bool control(const String& action, JsonObject* payload = nullptr);
    String getState();
    std::vector<int> getPins() const;
    String getId() const override { return _id; }
    String getType() const override { return _type; }
    String getName() const override { return _name; }
    void setStateChangeCallback(StateChangeCallback callback) override;
private:
    enum GateState { Closed, IsOpening, Opened, Closing };
    GateState _gateState = Closed;
    unsigned long _gateStateStart = 0;
    ServoDevice _servo;
    Button _sensor;
    Buzzer _buzzer;
    String _id;
    String _name;
    String _type = "GATE_WITH_SENSOR";
};

#endif // GATE_WITH_SENSOR_H
