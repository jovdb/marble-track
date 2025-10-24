#include "devices/Wheel.h"
#include "Logging.h"

Wheel::Wheel(const String &id)
    : Device(id, "wheel"), _stepper(nullptr), _sensor(nullptr)
{
    _direction = 1;
    _state = wheelState::IDLE;
}

void Wheel::setup()
{
    if (getChildren().empty())
    {
        // Create children if not loaded from config
        _stepper = new Stepper(getId() + "-stepper");
        _sensor = new Button(getId() + "-sensor");
        addChild(_stepper);
        addChild(_sensor);
    }
    else
    {
        // Set pointers from loaded children
        auto children = getChildren();
        if (children.size() >= 1)
            _stepper = static_cast<Stepper*>(children[0]);
        if (children.size() >= 2)
            _sensor = static_cast<Button*>(children[1]);
    }

    Device::setup(); // Call base setup to setup children
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

    if (!_stepper || !_sensor)
        return;

    // Started moving ?
    if (_stepper->isMoving() && _state != wheelState::MOVING)
    {
        _state = wheelState::MOVING;
        notifyStateChange();
    }
    // Stoped moving?
    else if (!_stepper->isMoving() && _state == wheelState::MOVING)
    {
        _state = wheelState::IDLE;
        notifyStateChange();
    }

    switch (_state)
    {
    case wheelState::CALIBRATING:
        if (_sensor->wasPressed())
        {
            MLOG_INFO("Wheel [%s]: Calibration complete.", getId().c_str());
            _stepper->stop(1000000);
            _stepper->setCurrentPosition(0);
        }
        break;

    default:
        break;
    }
}

bool Wheel::move(long steps)
{
    if (_stepper)
    {
        return _stepper->move(steps);
    }
    return false;
}

bool Wheel::calibrate()
{
    if (!_stepper)
        return false;

    MLOG_INFO("Wheel [%s]: Calibration started.", getId().c_str());
    _state = wheelState::CALIBRATING;
    notifyStateChange();
    return _stepper->move(100000 * _direction); // Move a large number of steps in the current direction
}

bool Wheel::control(const String &action, JsonObject *payload)
{
    // Use if-else if chain for string actions (C++ does not support switch on String)
    if (action == "next-breakpoint")
    {
        long steps = payload && (*payload)["steps"].is<long>() ? (*payload)["steps"].as<long>() : 5000;
        if (!move(steps))
            return false;
        return true;
    }
    else if (action == "calibrate")
    {
        if (!calibrate())
            return false;
        return true;
    }
    else if (action == "stop")
    {
        MLOG_INFO("Wheel [%s]: Stopping.", getId().c_str());
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

String Wheel::getConfig() const
{
    JsonDocument doc;
    // Copy base Device config fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getConfig());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    // Add wheel-specific config
    doc["name"] = _name;
    JsonArray arr = doc["breakPoints"].to<JsonArray>();
    for (long bp : _breakPoints)
    {
        arr.add(bp);
    }
    String result;
    serializeJson(doc, result);
    return result;
}

void Wheel::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Wheel [%s]: Null config provided", getId().c_str());
        return;
    }

    // Set name if provided
    if ((*config)["name"].is<String>())
    {
        _name = (*config)["name"].as<String>();
    }

    // Set breakPoints if provided
    if ((*config)["breakPoints"].is<JsonArray>())
    {
        _breakPoints.clear();
        JsonArray arr = (*config)["breakPoints"];
        for (JsonVariant v : arr)
        {
            if (v.is<long>())
            {
                _breakPoints.push_back(v.as<long>());
            }
        }
    }
}
