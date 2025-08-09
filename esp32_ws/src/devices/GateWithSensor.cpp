void GateWithSensor::setStateChangeCallback(StateChangeCallback callback) {
    Device::setStateChangeCallback(callback);
    _servo.setStateChangeCallback(callback);
    _sensor.setStateChangeCallback(callback);
}
#include "devices/GateWithSensor.h"

GateWithSensor::GateWithSensor(int servoPin, int servoPwmChannel, int buttonPin, const String& id, const String& name,
                                                             int initialAngle, bool buttonPullUp, unsigned long buttonDebounceMs, Button::ButtonType buttonType)
        : _servo(servoPin, id + "-servo", name + " Servo", 30, servoPwmChannel),
            _sensor(buttonPin, id + "-sensor", name + " Sensor", buttonPullUp, buttonDebounceMs, buttonType),
            _id(id), _name(name)
{
}

void GateWithSensor::setup() {
    _servo.setup();
    _sensor.setup();
}

void GateWithSensor::loop() {
    _servo.loop();
    _sensor.loop();
}

bool GateWithSensor::control(const String& action, JsonObject* payload) {
    return false;
}

String GateWithSensor::getState() {
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;
    String result;
    serializeJson(doc, result);
    return result;
}

std::vector<int> GateWithSensor::getPins() const {
    std::vector<int> pins;
    const auto& servoPins = _servo.getPins();
    const auto& sensorPins = _sensor.getPins();
    pins.insert(pins.end(), servoPins.begin(), servoPins.end());
    pins.insert(pins.end(), sensorPins.begin(), sensorPins.end());
    return pins;
}

