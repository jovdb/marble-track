
#include "devices/DividerWheel.h"
static const char *TAG = "DividerWheel";

DividerWheel::DividerWheel(int stepPin1, int stepPin2, int stepPin3, int stepPin4, int buttonPin, const String &id, const String &name)
    : Device(id, name, "DividerWheel")
{
    _stepper = new Stepper(stepPin1, stepPin2, stepPin3, stepPin4, id + "-stepper", name + " Stepper", 1000, 500);
    _button = new Button(buttonPin, id + "-button", name + " Button", true, 50);
    addChild(_stepper);
    addChild(_button);
}

DividerWheel::~DividerWheel()
{
    delete _stepper;
    delete _button;
}

void DividerWheel::loop()
{
    Device::loop();

    switch (_state)
    {
    case wheelState::CALIBRATING:
        if (_button->wasPressed())
        {
            ESP_LOGI(TAG, "Wheel [%s]: Calibration complete.", getId().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop();
            _state = wheelState::IDLE;
            notifyStateChange();
        }
        if (!_stepper->isMoving())
        {
            // Ended without calibrating button pressed
            // Check if button is connected
            // Check if button is pressed
        }
        break;

    default:
        break;
    }
}

void DividerWheel::setup()
{
    Device::setup();
}

void DividerWheel::move(long steps)
{
    if (_stepper)
    {
        _stepper->move(steps);
    }
}

void DividerWheel::calibrate()
{
    static float calibrationSpeed = 500;
    static float calibrationAcceleration = 200;

    if (_stepper)
    {
        ESP_LOGI(TAG, "Wheel [%s]: Calibration started.", getId().c_str());
        _state = wheelState::CALIBRATING;
        _stepper->setMaxSpeed(calibrationSpeed);
        _stepper->setAcceleration(calibrationAcceleration);
        _stepper->move(5000);
        notifyStateChange();
    }
}

bool DividerWheel::control(const String &action, JsonObject *payload)
{
    // Use if-else if chain for string actions (C++ does not support switch on String)
    if (action == "next-breakpoint")
    {
        long steps = payload && (*payload)["steps"].is<long>() ? (*payload)["steps"].as<long>() : 5000;
        move(steps);
        return true;
    }
    else if (action == "calibrate")
    {
        calibrate();
        return true;
    }
    else if (action == "stop")
    {
        ESP_LOGI(TAG, "Wheel [%s]: Stopping.", getId().c_str());
        _stepper->stop();
        return true;
    }
    // Add more actions here as needed
    return false;
}

String DividerWheel::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    doc["state"] = _state == wheelState::CALIBRATING ? "CALIBRATING" : "IDLE";
    String result;
    serializeJson(doc, result);
    return result;
}
