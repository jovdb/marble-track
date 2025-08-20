#include "esp_log.h"

static const char *TAG = "Wheel";
#include "devices/Wheel.h"

Wheel::Wheel(int stepPin1, int dirPin, int buttonPin, const String &id, const String &name)
    : Device(id, name, "wheel"),
      _stepper(new Stepper(stepPin1, dirPin, id + "-stepper", name + " Stepper", 100, 1000)),
      _sensor(new Button(buttonPin, id + "-sensor", name + " Sensor", true, 100, Button::ButtonType::NormalClosed)),
      _state(wheelState::IDLE)
{
    addChild(_stepper);
    addChild(_sensor);
}

// Array of breakpoints values
// these are the number of steps after the reset button
static const long breakpoints[] = {2000, 4000, 10000, 1200, 15000};

static enum class wheelState {
    CALIBRATING,
    IDLE,
    MOVING
} _state; ///< Current movement state

void Wheel::loop()
{
    Device::loop();

    if (_stepper && _stepper->isMoving() && _state != wheelState::MOVING)
    {
        //        _state = wheelState::MOVING;
        //        notifyStateChange();
    }
    else if (_stepper && !_stepper->isMoving() && _state == wheelState::MOVING)
    {
        _state = wheelState::IDLE;
        notifyStateChange();
    }

    switch (_state)
    {
    case wheelState::CALIBRATING:
        if (_sensor->wasPressed())
        {
            ESP_LOGI(TAG, "Wheel [%s]: Calibration complete.", getId().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop();
        }
        break;

    default:
        break;
    }
}

void Wheel::move(long steps)
{
    if (_stepper)
    {
        _stepper->move(steps);
    }
}

void Wheel::calibrate()
{
    if (_stepper)
    {
        ESP_LOGI(TAG, "Wheel [%s]: Calibration started.", getId().c_str());
        _state = wheelState::CALIBRATING;
        _stepper->move(100000);
        notifyStateChange();
    }
}

bool Wheel::control(const String &action, JsonObject *payload)
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

String Wheel::getState()
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
