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
            _stepper = static_cast<Stepper *>(children[0]);
        if (children.size() >= 2)
            _sensor = static_cast<Button *>(children[1]);
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
    if (_stepper->isMoving() && _state == wheelState::IDLE)
    {
        _state = wheelState::MOVING;
        notifyStateChange();
    }
    // Stopped moving?
    else if (!_stepper->isMoving() && _state == wheelState::MOVING)
    {
        // TODO: detect if no reset found during calibration?
        _state = wheelState::IDLE;
        notifyStateChange();
    }

    switch (_state)
    {
    case wheelState::RESET:
        if (_sensor->wasPressed())
        {
            MLOG_INFO("Wheel [%s]: Reset complete.", getId().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop(1000000); // Stop immediately
            _state = wheelState::IDLE;
            notifyStateChange();
        }
        break;
    case wheelState::CALIBRATING:
        if (_sensor->wasPressed())
        {
            // First phase: find zero
            if (_stateStep == 0)
            {
                MLOG_INFO("Wheel [%s]: Calibration: zero found, counting steps per revolution...", getId().c_str());
                _stepper->setCurrentPosition(0);
                _stateStep = 1;
            }
            // Second step: complete calibration
            else
            {
                long stepsPerRevolution = _stepper->getCurrentPosition();
                MLOG_INFO("Wheel [%s]: Calibration complete, step per revolution: %d", getId().c_str(), stepsPerRevolution);
                _stepper->setCurrentPosition(0);
                _stepper->stop();
                // TODO: move to first breakpoint

                notifyStepsPerRevolution(stepsPerRevolution);
            }
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

/** Goto the initial position, do one revolution to get the number of steps */
bool Wheel::calibrate()
{
    if (!_stepper)
        return false;

    MLOG_INFO("Wheel [%s]: Calibration started.", getId().c_str());
    _state = wheelState::CALIBRATING;
    _stateStep = 0;
    notifyStateChange();
    return _stepper->move(50000 * _direction); // Move a large number of steps in the current direction
}

/** Goto the initial position, until button is pressed*/
bool Wheel::reset()
{
    if (!_stepper)
        return false;

    MLOG_INFO("Wheel [%s]: Reset started.", getId().c_str());
    _state = wheelState::RESET;
    notifyStateChange();
    return _stepper->move(50000 * _direction); // Move a large number of steps in the current direction
}

void Wheel::notifyStepsPerRevolution(long steps)
{
    if (Device::notifyClients)
    {
        JsonDocument doc;
        doc["type"] = "steps-per-revolution";
        doc["deviceId"] = getId();
        doc["steps"] = steps;

        String message;
        serializeJson(doc, message);
        notifyClients(message);
    }
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
        return calibrate();
    }
    else if (action == "reset")
    {
        return reset();
    }
    else if (action == "stop")
    {
        _stepper->stop();
        return true;
    }
    // Add more actions here as needed
    return false;
}

String Wheel::stateToString(Wheel::wheelState state) const
{
    switch (state)
    {
    case Wheel::wheelState::CALIBRATING:
        return "CALIBRATING";
    case Wheel::wheelState::IDLE:
        return "IDLE";
    case Wheel::wheelState::MOVING:
        return "MOVING";
    case Wheel::wheelState::RESET:
        return "RESET";
    default:
        return "UNKNOWN";
    }
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
    doc["state"] = stateToString(_state);
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
