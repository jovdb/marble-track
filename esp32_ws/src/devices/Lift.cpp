#include "devices/Lift.h"
#include "Logging.h"

Lift::Lift(const String &id, NotifyClients notifyClients)
    : Device(id, "lift", notifyClients), _stepper(nullptr), _limitSwitch(nullptr), _ballSensor(nullptr), _loader(nullptr), _state(LiftState::UNKNOWN)
{
}

void Lift::setup()
{
    auto children = getChildren();
    if (children.empty())
    {
        // Create children if not loaded from config
        _stepper = new Stepper(getId() + "-stepper", _notifyClients);
        _limitSwitch = new Button(getId() + "-limit", _notifyClients);
        _ballSensor = new Button(getId() + "-ball-sensor", _notifyClients);
        _loader = new PwmMotor(getId() + "-loader", _notifyClients);
        addChild(_stepper);
        addChild(_limitSwitch);
        addChild(_ballSensor);
        addChild(_loader);
    }
    else
    {
        // Set pointers from loaded children
        if (children.size() >= 1)
            _stepper = static_cast<Stepper *>(children[0]);
        if (children.size() >= 2)
            _limitSwitch = static_cast<Button *>(children[1]);
        if (children.size() >= 3)
            _ballSensor = static_cast<Button *>(children[2]);
        if (children.size() >= 4)
            _loader = static_cast<PwmMotor *>(children[3]);
    }

    // Call base setup to setup children
    Device::setup();
}

void Lift::loop()
{
    Device::loop();

    if (!_stepper)
        return;

    switch (_state)
    {
    case LiftState::UNKNOWN:
    {
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            _stepper->setCurrentPosition(0);
            _state = (_ballSensor && _ballSensor->isPressed()) ? LiftState::BALL_WAITING : LiftState::IDLE;
            notifyStateChange();
        }
        break;
    }
    case LiftState::IDLE:
        if (_ballSensor && _ballSensor->isPressed())
        {
            MLOG_INFO("Lift [%s]: Ball loaded", getId().c_str());
            _state = LiftState::BALL_WAITING;
            notifyStateChange();
        }
        break;
    case LiftState::RESET:
        // During reset, check if limit switch is pressed
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            MLOG_INFO("Lift [%s]: Reset complete - limit switch pressed", getId().c_str());
            // _stepper->stop(100000); // Stop the stepper
            _stepper->setCurrentPosition(0); // Reset position to zero
            _state = (_ballSensor && _ballSensor->isPressed()) ? LiftState::BALL_WAITING : LiftState::IDLE;
            notifyStateChange();
        }
        else if (_stepper && !_stepper->isMoving())
        {
            MLOG_WARN("Lift [%s]: Reset incomplete - Lift stopped but limit switch not pressed", getId().c_str());
            // ERROR vs IDLE
            _state = LiftState::IDLE;
            notifyStateChange();
        }
        break;
    case LiftState::BALL_WAITING:
        if (_ballSensor && _ballSensor->onReleased())
        {
            MLOG_INFO("Lift [%s]: Ball unloaded", getId().c_str());
            _state = LiftState::IDLE;
            notifyStateChange();
        }

        if (_loader->getValue() > 90)
        {
            _state = LiftState::LIFT_LOADED;
            notifyStateChange();
        }
        break;
    case LiftState::LIFT_LOADED:
        break;
    case LiftState::MOVING_UP:
    case LiftState::MOVING_DOWN:
        // When stepper stops moving, return to appropriate idle state
        if (_stepper && !_stepper->isMoving())
        {
            MLOG_INFO("Lift [%s]: Movement complete", getId().c_str());
            _state = (_ballSensor && _ballSensor->isPressed()) ? LiftState::BALL_WAITING : LiftState::IDLE;
            notifyStateChange();
        }

        if (_limitSwitch && _limitSwitch->isPressed() && _state == LiftState::MOVING_DOWN)
        {
            MLOG_WARN("Lift [%s]: Limit switch triggered during downward movement - stopping", getId().c_str());
            _stepper->setCurrentPosition(0); // Reset position to zero
            _state = (_ballSensor && _ballSensor->isPressed()) ? LiftState::BALL_WAITING : LiftState::IDLE;
            notifyStateChange();
        }
        break;
    }
}

bool Lift::up()
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    // Prevent operation when state is unknown or error
    if (_state == LiftState::UNKNOWN || _state == LiftState::ERROR)
    {
        MLOG_WARN("Lift [%s]: Cannot move up - state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;
    }

    // Check if lift is already at or above max position
    long currentPos = _stepper->getCurrentPosition();
    if (currentPos <= -_maxSteps)
    {
        MLOG_WARN("Lift [%s]: Cannot move up - already at max position (current: %ld, max: %ld)", getId().c_str(), currentPos, -_maxSteps);
        return false;
    }

    MLOG_INFO("Lift [%s]: Moving up to %ld steps", getId().c_str(), _maxSteps);
    _state = LiftState::MOVING_UP;
    notifyStateChange();
    return _stepper->moveTo(-_maxSteps);
}

bool Lift::down()
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    // Prevent operation when state is unknown or error
    if (_state == LiftState::UNKNOWN || _state == LiftState::ERROR)
    {
        MLOG_WARN("Lift [%s]: Cannot move down - state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;
    }

    // Check if lift is already at or below min position
    long currentPos = _stepper->getCurrentPosition();
    if (currentPos >= -_minSteps)
    {
        MLOG_WARN("Lift [%s]: Cannot move down - already at min position (current: %ld, min: %ld)", getId().c_str(), currentPos, -_minSteps);
        return false;
    }

    MLOG_INFO("Lift [%s]: Moving down to %ld steps", getId().c_str(), _minSteps);
    _state = LiftState::MOVING_DOWN;
    notifyStateChange();
    return _stepper->moveTo(-_minSteps);
}

bool Lift::reset()
{
    if (!_stepper || !_limitSwitch)
    {
        MLOG_WARN("Lift [%s]: Stepper or limit switch not initialized", getId().c_str());
        return false;
    }

    if (_limitSwitch->isPressed())
    {
        MLOG_INFO("Lift [%s]: Altready on limit", getId().c_str());

        _state = LiftState::IDLE;
        notifyStateChange();
        return true;
    }

    MLOG_INFO("Lift [%s]: Starting reset - moving down slowly until limit switch is pressed", getId().c_str());
    _state = LiftState::RESET;

    // Slowly close the gate
    _loader->setValue(0.0f, 3000);

    // Move down slowly (negative direction) until limit switch is pressed
    // We'll move in small steps and check the limit switch
    bool startedMove = _stepper->move(_maxSteps - _minSteps, _stepper ? _stepper->_defaultSpeed / 3 : 100);

    notifyStateChange();

    return startedMove;

    // TODO:
    // When reset, go up and empty the lift
}

bool Lift::loadBallStart()
{
    if (!_loader)
    {
        MLOG_WARN("Lift [%s]: Lift Loader not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Setting loader to load ball end (0)", getId().c_str());
    _state = LiftState::LIFT_LOADED;
    notifyStateChange();

    // Set loader to 100 (fully open)
    return _loader->setValue(100.0f);
}

bool Lift::loadBallEnd()
{
    if (!_loader)
    {
        MLOG_WARN("Lift [%s]: Lift Loader not initialized", getId().c_str());
        return false;
    }

    _state = LiftState::LIFT_LOADED;
    notifyStateChange();

    // Set loader to 0 (fully closed)
    return _loader->setValue(0.0f);
}

bool Lift::control(const String &action, JsonObject *payload)
{
    if (action == "up")
    {
        return up();
    }
    else if (action == "down")
    {
        return down();
    }
    else if (action == "reset")
    {
        return reset();
    }
    else if (action == "loadBallStart")
    {
        return loadBallStart();
    }
    else if (action == "loadBallEnd")
    {
        return loadBallEnd();
    }
    else
    {
        MLOG_WARN("Lift [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
    }

    return false;
}

String Lift::stateToString(Lift::LiftState state) const
{
    switch (state)
    {
    case Lift::LiftState::UNKNOWN:
        return "Unknown";
    case Lift::LiftState::IDLE:
        return "Idle";
    case Lift::LiftState::BALL_WAITING:
        return "BallWaiting";
    case Lift::LiftState::RESET:
        return "Reset";
    case Lift::LiftState::ERROR:
        return "Error";
    case Lift::LiftState::LIFT_LOADED:
        return "LiftLoaded";
    case Lift::LiftState::MOVING_UP:
        return "MovingUp";
    case Lift::LiftState::MOVING_DOWN:
        return "MovingDown";
    default:
        return "UNKNOWN";
    }
}

String Lift::getState()
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
    if (_stepper)
    {
        doc["currentPosition"] = _stepper->getCurrentPosition();
    }

    String result;
    serializeJson(doc, result);
    return result;
}

String Lift::getConfig() const
{
    JsonDocument doc;
    // Copy base Device config fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getConfig());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    // Add lift-specific config
    doc["name"] = _name;
    doc["minSteps"] = _minSteps;
    doc["maxSteps"] = _maxSteps;

    String result;
    serializeJson(doc, result);
    return result;
}

void Lift::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Lift [%s]: Null config provided", getId().c_str());
        return;
    }

    // Set name if provided
    if ((*config)["name"].is<String>())
    {
        const String name = (*config)["name"].as<String>();
        _name = name;
    }

    // Set minSteps if provided
    if ((*config)["minSteps"].is<long>())
    {
        const long minSteps = (*config)["minSteps"].as<long>();
        _minSteps = minSteps;
    }

    // Set maxSteps if provided
    if ((*config)["maxSteps"].is<long>())
    {
        const long maxSteps = (*config)["maxSteps"].as<long>();
        _maxSteps = maxSteps;
    }
}