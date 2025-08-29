#include <ArduinoJson.h>
// ...existing code...
#include "devices/GateWithSensor.h"
#include "devices/Buzzer.h"

GateWithSensor::GateWithSensor(int servoPin, int servoPwmChannel, int buttonPin, Buzzer *buzzer, const String &id, const String &name,
                               int initialAngle, bool buttonPullUp, unsigned long buttonDebounceMs, Button::ButtonType buttonType)
    : Device(id, name, "gate"), _servo(new ServoDevice(id + "-servo", name + " Servo")),
      _sensor(new Button(id + "-sensor", name + " Sensor")),
      _buzzer(buzzer),
      _gateState(Closed),
      _gateStateStart(0)
{
    addChild(_servo);
    addChild(_sensor);
}

void GateWithSensor::loop()
{
    Device::loop();

    switch (_gateState)
    {
    case Closed:
        if (_sensor && _sensor->wasPressed())
        {
            open();
        }
        break;
    case IsOpening:
        if (millis() - _gateStateStart >= 400)
        { // Wait for opening
            _gateState = Opened;
            _gateStateStart = millis();
            notifyStateChange();
        }
        break;
    case Opened:
        // Stay open for a while, then start closing
        if (millis() - _gateStateStart >= 400)
        {
            if (_servo)
            {
                _servo->setSpeed(200);
                _servo->setAngle(30); // Close gate
            }
            _gateState = Closing;
            _gateStateStart = millis();
            notifyStateChange();
        }
        break;
    case Closing:
        if (millis() - _gateStateStart >= 1000)
        {
            _gateState = Closed;
            notifyStateChange();
        }
        break;
    }
}

void GateWithSensor::open()
{
    if (_gateState == Closed)
    {
        if (_buzzer)
            _buzzer->tone(500, 100);
        if (_servo)
        {
            _servo->setSpeed(240);
            _servo->setAngle(170); // Open gate
        }
        _gateState = IsOpening;
        _gateStateStart = millis();
        notifyStateChange();
    }
}

String GateWithSensor::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
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

bool GateWithSensor::control(const String &action, JsonObject *payload)
{
    // Default implementation, extend as needed
    return false;
}