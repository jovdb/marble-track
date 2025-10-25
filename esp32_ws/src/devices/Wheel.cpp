#include "devices/Wheel.h"
#include "Logging.h"

long maxStepsPerRevolution = 50000;
// 34580
// 34587

// Array of breakpoints values
// these are the number of steps after the reset button
static const long breakpoints[] = {2000, 4000, 10000, 1200, 15000};

Wheel::Wheel(const String &id)
    : Device(id, "wheel"), _stepper(nullptr), _sensor(nullptr), _stepsInLastRevolution(0)
{
    _direction = 1;
    _state = wheelState::IDLE;
}

void Wheel::setup()
{
    auto children = getChildren();
    MLOG_WARN("Wheel [%s]: Setup start, children count: %d", getId().c_str(), children.size());
    if (children.empty())
    {
        // Create children if not loaded from config
        _stepper = new Stepper(getId() + "-stepper");
        _sensor = new Button(getId() + "-sensor");
        addChild(_stepper);
        addChild(_sensor);

        // Initialize breakpoints if not loaded from config
        if (_breakPoints.empty())
        {
            _breakPoints.assign(std::begin(breakpoints), std::end(breakpoints));
        }
    }
    else
    {
        // Set pointers from loaded children
        if (children.size() >= 1)
            _stepper = static_cast<Stepper *>(children[0]);
        if (children.size() >= 2)
            _sensor = static_cast<Button *>(children[1]);
    }

    // Call base setup to setup children
    Device::setup();

    MLOG_WARN("Wheel [%s]: Setup end, children count: %d", getId().c_str(), children.size());
}

void Wheel::loop()
{
    Device::loop();

    if (!_stepper || !_sensor)
        return;

    switch (_state)
    {
    case wheelState::IDLE:
    {
        // Started moving
        if (_stepper->isMoving())
        {
            _state = wheelState::MOVING;
            notifyStateChange();
        }
        break;
    }
    case wheelState::MOVING:
        // Stopped moving
        if (!_stepper->isMoving())
        {
            _state = wheelState::IDLE;

            // Reset position to avoid overflow
            // if (_lastZeroPosition > 0)
            // {
            //     MLOG_INFO("Wheel [%s]: Resetting current position to avoid overflow.", getId().c_str());
            //     long currentPos = _stepper->getCurrentPosition();

            //     // TODO: modulo when rotating in the negative direction
            //     if (currentPos > _stepsInLastRevolution)
            //     {
            //         long newPosition = currentPos % _stepsInLastRevolution;

            //         _stepper->setCurrentPosition(newPosition);
            //         _lastZeroPosition = newPosition;
            //     }
            // }
            notifyStateChange();
        }
        break;
    case wheelState::RESET:
        if (_sensor->wasPressed())
        {
            MLOG_INFO("Wheel [%s]: Reset complete.", getId().c_str());
            _lastZeroPosition = _stepper->getCurrentPosition();

            // TODO -> Move to first breakpoint?
            _stepper->stop();

            // Resting to moving
            _state = wheelState::MOVING;
            notifyStateChange();
        }

        // No zero found during calibration?
        // TODO: Wait for started moving
        // if (!_stepper->isMoving())
        // {
        //     _state = wheelState::IDLE;
        //     notifyStateChange();
        //     notifyError("reset-error", "No zero point found during reset.");
        // }

        break;
    case wheelState::CALIBRATING:
        if (_sensor->wasPressed())
        {
            // First phase: find zero
            if (_lastZeroPosition == 0)
            {
                MLOG_INFO("Wheel [%s]: Calibration: zero found, counting steps per revolution...", getId().c_str());
                _lastZeroPosition = _stepper->getCurrentPosition();
            }
            // Second step: complete calibration
            else
            {
                long currentPos = _stepper->getCurrentPosition();
                _stepsInLastRevolution = currentPos - _lastZeroPosition;
                MLOG_INFO("Wheel [%s]: Calibration complete, step per revolution: %d", getId().c_str(), _stepsInLastRevolution);
                _lastZeroPosition = currentPos;
                _stepper->stop();

                // TODO: move to first breakpoint
                notifyStepsPerRevolution(_stepsInLastRevolution);

                // Calibrating to moving
                _state = wheelState::MOVING;
                notifyStateChange();
            }
        }

        // No zero found during calibration?
        // TODO: Wait for started moving
        // if (!_stepper->isMoving())
        // {
        //     _state = wheelState::IDLE;
        //     notifyStateChange();
        //     notifyError("calibration-error", "No zero point found during calibration.");
        // }

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
    {
        MLOG_WARN("Wheel [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Wheel [%s]: Calibration started.", getId().c_str());
    _state = wheelState::CALIBRATING;
    _lastZeroPosition = 0;
    notifyStateChange();
    // Max 2 revolutions
    return _stepper->move(maxStepsPerRevolution * 2 * _direction); // Move a large number of steps in the current direction
}

/** Goto the initial position, until button is pressed*/
bool Wheel::reset()
{
    if (!_stepper)
        return false;

    MLOG_INFO("Wheel [%s]: Reset started.", getId().c_str());
    _state = wheelState::RESET;
    notifyStateChange();
    return _stepper->move(maxStepsPerRevolution * _direction); // Move a large number of steps in the current direction
}

/** Move to a specific angle (0-359.9 degrees) - absolute positioning based on zero point */
bool Wheel::moveToAngle(float angle)
{
    if (!_stepper)
    {
        MLOG_WARN("Wheel [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    if (_stepsPerRevolution <= 0)
    {
        MLOG_WARN("Wheel [%s]: Cannot move to angle - steps per revolution not calibrated", getId().c_str());
        return false;
    }

    if (_lastZeroPosition == 0)
    {
        MLOG_WARN("Wheel [%s]: Cannot move to angle - zero point not set (run calibration or reset first)", getId().c_str());
        return false;
    }

    // Ensure angle is in valid range
    while (angle < 0)
        angle += 360;
    while (angle >= 360)
        angle -= 360;

    // Calculate absolute target position relative to zero point
    long targetPosition = _lastZeroPosition + (angle / 360.0) * _stepsPerRevolution;
    long currentPosition = _stepper->getCurrentPosition();
    long stepsToMove = targetPosition - currentPosition;

    MLOG_INFO("Wheel [%s]: Moving to absolute angle %.1f° (target pos: %ld, current pos: %ld, steps: %ld)",
              getId().c_str(), angle, targetPosition, currentPosition, stepsToMove);

    return _stepper->move(stepsToMove);
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
    else if (action == "move-to-angle")
    {
        float angle = payload && (*payload)["angle"].is<float>() ? (*payload)["angle"].as<float>() : 0.0f;
        return moveToAngle(angle);
    }
    else if (action == "stop")
    {
        _stepper->stop();
        return true;
    }
    else
    {
        MLOG_WARN("Wheel [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
    }

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
    doc["lastZeroPosition"] = _lastZeroPosition;
    doc["stepsInLastRevolution"] = _stepsInLastRevolution;
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
    doc["stepsPerRevolution"] = _stepsPerRevolution;
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

    // Set stepsPerRevolution if provided
    if ((*config)["stepsPerRevolution"].is<long>())
    {
        _stepsPerRevolution = (*config)["stepsPerRevolution"].as<long>();
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
