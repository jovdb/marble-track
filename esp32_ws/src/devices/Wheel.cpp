#include "devices/Wheel.h"

Wheel::Wheel(int pin1, int pin2, int pin3, int pin4, int buttonPin, const String &id, const String &name)
    : Device(name, "MARBLE_WHEEL"), _stepper(new Stepper(pin1, pin2, pin3, pin4, id + "-stepper", name + " Stepper", 100, 1000)),
      _sensor(new Button(buttonPin, id + "-sensor", name + " Sensor", true, 100, Button::ButtonType::NormalClosed)),
      _id(id)
{
    addChild(_stepper);
    addChild(_sensor);
}

void Wheel::loop()
{
    Device::loop();
    // Example: spin wheel if button pressed
    if (_sensor && _sensor->wasPressed())
    {
        spin(200); // Spin 200 steps when button pressed
    }
}

void Wheel::spin(long steps)
{
    if (_stepper)
    {
        _stepper->move(steps);
    }
}

bool Wheel::control(const String &action, JsonObject *payload)
{
    if (action == "next-breakpoint")
    {
        long steps = payload && (*payload)["steps"].is<long>() ? (*payload)["steps"].as<long>() : 5000;
        spin(steps);
        return true;
    }
    return false;
}

String Wheel::getState()
{
    JsonDocument doc;
    doc["type"] = getType();
    doc["name"] = getName();
    doc["stepperPosition"] = _stepper ? _stepper->getCurrentPosition() : 0;
    doc["buttonPressed"] = _sensor ? _sensor->wasPressed() : false;
    String result;
    serializeJson(doc, result);
    return result;
}
