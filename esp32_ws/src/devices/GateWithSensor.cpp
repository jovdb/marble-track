#include "devices/GateWithSensor.h"
#include "devices/Buzzer.h"

void GateWithSensor::open()
{
    if (_gateState == Closed)
    {
        _buzzer.tone(500, 100);
        _servo->setSpeed(240);
        _servo->setAngle(170); // Open gate
        _gateState = IsOpening;
        _gateStateStart = millis();
    }
}

GateWithSensor::GateWithSensor(int servoPin, int servoPwmChannel, int buttonPin, int buzzerPin, const String &id, const String &name,
                               int initialAngle, bool buttonPullUp, unsigned long buttonDebounceMs, Button::ButtonType buttonType)
    : _servo(new ServoDevice(servoPin, id + "-servo", name + " Servo", 30, servoPwmChannel)),
      _sensor(new Button(buttonPin, id + "-sensor", name + " Sensor", buttonPullUp, buttonDebounceMs, buttonType)),
      _buzzer(buzzerPin, id + "-buzzer", name + " Buzzer"),
      _id(id), _name(name), _gateState(Closed), _gateStateStart(0)
{
    addChild(_servo);
    addChild(_sensor);
}

void GateWithSensor::loop()
{
    Device::loop(); // Call super loop to process children

    switch (_gateState)
    {
    case Closed:
        if (_sensor && _sensor->wasPressed())
        {
            open();
        }
        break;
    case IsOpening:
        if (millis() - _gateStateStart >= 1000)
        { // Wait for opening
            _gateState = Opened;
            _gateStateStart = millis();
        }
        break;
    case Opened:
        // Stay open for a while, then start closing
        if (millis() - _gateStateStart >= 1000)
        {
            if (_servo)
            {
                _servo->setSpeed(200);
                _servo->setAngle(30); // Close gate
            }
            _gateState = Closing;
            _gateStateStart = millis();
        }
        break;
    case Closing:
        if (millis() - _gateStateStart >= 1000)
        {
            _gateState = Closed;
        }
        break;
    }
}

bool GateWithSensor::control(const String &action, JsonObject *payload)
{
    if (action == "Open")
    {
        open();
        return true;
    }
    return false;
}

String GateWithSensor::getState()
{
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;
    const char *stateStr = "Unknown";
    switch (_gateState)
    {
    case Closed:
        stateStr = "Closed";
        break;
    case IsOpening:
        stateStr = "IsOpening";
        break;
    case Opened:
        stateStr = "Opened";
        break;
    case Closing:
        stateStr = "Closing";
        break;
    }
    doc["gateState"] = stateStr;
    String result;
    serializeJson(doc, result);
    return result;
}