
#include "devices/GateWithSensor.h"
#include "devices/Buzzer.h"
void GateWithSensor::setStateChangeCallback(StateChangeCallback callback)
{
    Device::setStateChangeCallback(callback);
    _servo.setStateChangeCallback(callback);
    _sensor.setStateChangeCallback(callback);
}

GateWithSensor::GateWithSensor(int servoPin, int servoPwmChannel, int buttonPin, int buzzerPin, const String &id, const String &name,
                                                             int initialAngle, bool buttonPullUp, unsigned long buttonDebounceMs, Button::ButtonType buttonType)
        : _servo(servoPin, id + "-servo", name + " Servo", 30, servoPwmChannel),
            _sensor(buttonPin, id + "-sensor", name + " Sensor", buttonPullUp, buttonDebounceMs, buttonType),
            _buzzer(buzzerPin, id + "-buzzer", name + " Buzzer"),
            _id(id), _name(name)
{
}

void GateWithSensor::setup()
{
    _servo.setup();
    _sensor.setup();
    _buzzer.setup();
}

void GateWithSensor::loop()
{
    _servo.loop();
    _sensor.loop();
    _buzzer.loop();

    static unsigned long actionStart = 0;
    static int actionStep = 0;
    if (_sensor.wasPressed()) {
        _buzzer.tone(500, 100);
        actionStart = millis();
        actionStep = 1;
    }
    if (actionStep == 1 && millis() - actionStart >= 100) {
        _servo.setSpeed(240);
        _servo.setAngle(170);
        actionStart = millis();
        actionStep = 2;
    }
    if (actionStep == 2 && millis() - actionStart >= 1000) {
        _servo.setSpeed(200);
        _servo.setAngle(30);
        actionStep = 0;
    }
}

bool GateWithSensor::control(const String &action, JsonObject *payload)
{
    return false;
}

String GateWithSensor::getState()
{
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;
    String result;
    serializeJson(doc, result);
    return result;
}

std::vector<int> GateWithSensor::getPins() const
{
    std::vector<int> pins;
    const auto &servoPins = _servo.getPins();
    const auto &sensorPins = _sensor.getPins();
    pins.insert(pins.end(), servoPins.begin(), servoPins.end());
    pins.insert(pins.end(), sensorPins.begin(), sensorPins.end());
    return pins;
}
