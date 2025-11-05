#include "devices/Lift.h"
#include "Logging.h"

Lift::Lift(const String &id, NotifyClients notifyClients)
    : Device(id, "lift", notifyClients), _stepper(nullptr), _limitSwitch(nullptr), _ballSensor(nullptr), _loader(nullptr), _unloader(nullptr), _state(LiftState::UNKNOWN)
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
        _unloader = new PwmMotor(getId() + "-unloader", _notifyClients);
        addChild(_stepper);
        addChild(_limitSwitch);
        addChild(_ballSensor);
        addChild(_loader);
        addChild(_unloader);
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
        if (children.size() >= 5)
            _unloader = static_cast<PwmMotor *>(children[4]);
    }

    // Call base setup to setup children
    Device::setup();
}

void Lift::loop()
{
    Device::loop();

    if (!_stepper)
        return;

    // Check ball sensor state and notify if changed
    bool ballWaiting = _ballSensor && _ballSensor->isPressed();
    if (ballWaiting != _isBallWaiting)
    {
        _isBallWaiting = ballWaiting;
        MLOG_INFO("Lift [%s]: Ball waiting state changed to %s", getId().c_str(), ballWaiting ? "true" : "false");
        notifyStateChange();
    }

    switch (_state)
    {
    case LiftState::UNKNOWN:
    {
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            _stepper->setCurrentPosition(0);
            _state = LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }
        break;
    }
    case LiftState::RESET:
        // During reset, check if limit switch is pressed
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            MLOG_INFO("Lift [%s]: Reset complete - limit switch pressed", getId().c_str());
            _stepper->setCurrentPosition(0);      // Reset position to zero
            _state = LiftState::LIFT_DOWN_LOADED; // First unload?
            notifyStateChange();
        }
        else if (_stepper && !_stepper->isMoving())
        {
            MLOG_WARN("Lift [%s]: Reset incomplete - Lift stopped but limit switch not pressed", getId().c_str());
            _state = LiftState::ERROR;
            notifyStateChange();
        }
        break;
    case LiftState::ERROR:
        // In error state, do nothing - requires manual reset
        break;
    case LiftState::LIFT_DOWN_LOADING:
        // Wait 1 second after starting load, then end the loading process
        if (millis() - _loadStartTime >= 1000)
        {
            loadBallEnd();
        }
        break;
    case LiftState::LIFT_DOWN_LOADED:
        break;
    case LiftState::LIFT_UP_UNLOADING:
        // Wait 2 seconds after starting unload, then end the unloading process
        if (millis() - _unloadStartTime >= 2000)
        {
            unloadBallEnd();
        }
        break;
    case LiftState::LIFT_UP_UNLOADED:
        break;
    case LiftState::LIFT_UP_LOADED:
        break;
    case LiftState::LIFT_DOWN_UNLOADED:
        break;
    case LiftState::MOVING_UP:
        if (_stepper && !_stepper->isMoving())
        {
            MLOG_INFO("Lift [%s]: Top reached", getId().c_str());
            _state = _isLoaded ? LiftState::LIFT_UP_LOADED : LiftState::LIFT_UP_UNLOADED;
            notifyStateChange();
        }
        break;
    case LiftState::MOVING_DOWN:
        // When stepper stops moving, return to appropriate idle state
        if (_stepper && !_stepper->isMoving())
        {
            MLOG_INFO("Lift [%s]: Movement complete", getId().c_str());
            _state = _isLoaded ? LiftState::LIFT_DOWN_LOADED : LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }

        else if (_limitSwitch && _limitSwitch->isPressed() && _state == LiftState::MOVING_DOWN)
        {
            MLOG_WARN("Lift [%s]: Limit switch triggered during downward movement - stopping", getId().c_str());
            _stepper->setCurrentPosition(0); // Reset position to zero
            _state = LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }
        break;
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in loop()", getId().c_str());
    }
}

bool Lift::up()
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    switch (_state)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::MOVING_UP:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    case LiftState::LIFT_UP_UNLOADING:
        MLOG_WARN("Lift [%s]: Cannot move up, state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;

    case LiftState::LIFT_DOWN_UNLOADED:
    case LiftState::LIFT_DOWN_LOADED:
    case LiftState::MOVING_DOWN:
    {
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
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in up()", getId().c_str());
        return false;
    }
}

bool Lift::down()
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    switch (_state)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::MOVING_DOWN:
    case LiftState::LIFT_DOWN_UNLOADED:
    case LiftState::LIFT_DOWN_LOADED:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADING:
        MLOG_WARN("Lift [%s]: Cannot move down, state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;

    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    case LiftState::MOVING_UP:
    {
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
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in down()", getId().c_str());
        return false;
    }
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
        MLOG_INFO("Lift [%s]: Already on limit", getId().c_str());

        _state = LiftState::LIFT_DOWN_LOADED; // first UNLOAD?
        notifyStateChange();
        return true;
    }

    MLOG_INFO("Lift [%s]: Starting reset - moving down slowly until limit switch is pressed", getId().c_str());
    _state = LiftState::RESET;

    // Slowly close the gate
    _loader->setValue(0.0f, 3000);

    // Move down slowly (negative direction) until limit switch is pressed
    // We'll move in small steps and check the limit switch
    bool startedMove = _stepper->move((_maxSteps - _minSteps) * 1.1, _stepper ? _stepper->_defaultSpeed / 3 : 100);

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

    MLOG_INFO("Lift [%s]: Loading ball...", getId().c_str());
    _state = LiftState::LIFT_DOWN_LOADING;
    _loadStartTime = millis();
    _isLoaded = true;
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

    _state = LiftState::LIFT_DOWN_LOADED;
    notifyStateChange();

    // Set loader to 0 (fully closed)
    return _loader->setValue(0.0f);
}

bool Lift::unloadBallStart()
{
    if (!_unloader)
    {
        MLOG_WARN("Lift [%s]: Lift Unloader not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Unloading ball...", getId().c_str());
    _state = LiftState::LIFT_UP_UNLOADING;
    _unloadStartTime = millis();
    _isLoaded = false;
    notifyStateChange();

    // Set unloader to 100 (fully open)
    return _unloader->setValue(100.0f);
}

bool Lift::unloadBallEnd()
{
    if (!_unloader)
    {
        MLOG_WARN("Lift [%s]: Lift Unloader not initialized", getId().c_str());
        return false;
    }

    _state = LiftState::LIFT_UP_UNLOADED;
    notifyStateChange();

    // Set unloader to 0 (fully closed)
    return _unloader->setValue(0.0f);
}

bool Lift::loadBall()
{
    switch (_state)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::MOVING_UP:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    case LiftState::LIFT_UP_UNLOADING:
    case LiftState::MOVING_DOWN:
    case LiftState::LIFT_DOWN_LOADED:
        MLOG_WARN("Lift [%s]: Cannot load ball, state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;

    case LiftState::LIFT_DOWN_UNLOADED:
    {
        return loadBallStart();
    }
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in loadBall()", getId().c_str());
        return false;
    }
}

bool Lift::unloadBall()
{
    switch (_state)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::MOVING_UP:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADING:
    case LiftState::MOVING_DOWN:
    case LiftState::LIFT_DOWN_LOADED:
    case LiftState::LIFT_DOWN_UNLOADED:
        MLOG_WARN("Lift [%s]: Cannot unload ball, state is %s", getId().c_str(), stateToString(_state).c_str());
        return false;

    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    {
        return unloadBallStart();
    }
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in unloadBall()", getId().c_str());
        return false;
    }
}

bool Lift::isBallWaiting()
{
    return _isBallWaiting;
}

bool Lift::isLoaded()
{
    return _isLoaded;
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
    else if (action == "loadBall")
    {
        return loadBall();
    }
    else if (action == "unloadBall")
    {
        return unloadBall();
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
    case Lift::LiftState::ERROR:
        return "Error";
    case Lift::LiftState::RESET:
        return "Reset";
    case Lift::LiftState::LIFT_DOWN_LOADING:
        return "LiftDownLoading";
    case Lift::LiftState::LIFT_DOWN_LOADED:
        return "LiftDownLoaded";
    case Lift::LiftState::LIFT_UP_UNLOADING:
        return "LiftUpUnloading";
    case Lift::LiftState::LIFT_UP_UNLOADED:
        return "LiftUpUnloaded";
    case Lift::LiftState::LIFT_UP_LOADED:
        return "LiftUpLoaded";
    case Lift::LiftState::LIFT_DOWN_UNLOADED:
        return "LiftDownUnloaded";
    case Lift::LiftState::MOVING_UP:
        return "MovingUp";
    case Lift::LiftState::MOVING_DOWN:
        return "MovingDown";
    default:
        return "Unknown";
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
    doc["isBallWaiting"] = isBallWaiting();

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