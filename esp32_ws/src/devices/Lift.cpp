#include "devices/Lift.h"
#include "Logging.h"

Lift::Lift(const String &id, NotifyClients notifyClients)
    : Device(id, "lift", notifyClients), _stepper(nullptr), _limitSwitch(nullptr), _ballSensor(nullptr), _loader(nullptr), _unloader(nullptr), liftState(LiftState::UNKNOWN)
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

    switch (liftState)
    {
    case LiftState::UNKNOWN:
    {
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            _stepper->setCurrentPosition(0);
            liftState = LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }
        break;
    }
    case LiftState::RESET:
        // Handle reset sequence steps
        if (_resetStep == 1) // Move unload out of the way first
        {
            _unloader->setValue(0.0f);
            _unloadEndTime = millis();
            _resetStep = 2; // waiting 1s
        }
        else if (_resetStep == 2) // Waiting after unload end
        {
            if (millis() - _unloadEndTime >= 1000)
            {
                if (_limitSwitch->isPressed())
                {
                    MLOG_INFO("Lift [%s]: Already on limit, moving up", getId().c_str());
                    _stepper->setCurrentPosition(0);
                    _stepper->moveTo(_direction * _maxSteps);
                    _resetStep = 4; // moving up
                }
                else
                {
                    MLOG_INFO("Lift [%s]: Moving down to limit", getId().c_str());
                    _stepper->move(_direction * (_minSteps - _maxSteps), _stepper->getDefaultSpeed() * 0.25);
                    _resetStep = 3; // moving down
                }
            }
        }
        else if (_resetStep == 3) // Moving down to limit
        {
            if (_limitSwitch && _limitSwitch->isPressed())
            {
                MLOG_INFO("Lift [%s]: Limit switch reached, moving up", getId().c_str());
                _stepper->setCurrentPosition(0);
                _stepper->moveTo(_direction * _maxSteps);
                _resetStep = 4;
            }
            else if (_stepper && !_stepper->isMoving())
            {
                MLOG_WARN("Lift [%s]: Reset failed - stopped moving but limit not pressed", getId().c_str());
                liftState = LiftState::ERROR;
                _resetStep = 0;
                notifyStateChange();
            }
        }
        else if (_resetStep == 4) // Moving up
        {
            if (_stepper && !_stepper->isMoving())
            {
                MLOG_INFO("Lift [%s]: At top, unloading", getId().c_str());
                _unloader->setValue(100.0f);
                _unloadStartTime = millis();
                _resetStep = 5;
            }
        }
        else if (_resetStep == 5) // Unloading
        {
            if (millis() - _unloadStartTime >= 2000)
            {
                MLOG_INFO("Lift [%s]: Unload complete, moving down", getId().c_str());
                _unloader->setValue(0.0f);
                _isLoaded = false;
                _stepper->moveTo(_direction * _minSteps, _stepper->_defaultSpeed);
                _resetStep = 6;
            }
        }
        else if (_resetStep == 6) // Moving down
        {
            if (_stepper && !_stepper->isMoving())
            {
                MLOG_INFO("Lift [%s]: Reset complete", getId().c_str());
                liftState = LiftState::LIFT_DOWN_UNLOADED;
                _resetStep = 0;
                notifyStateChange();
            }
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
            liftState = _isLoaded ? LiftState::LIFT_UP_LOADED : LiftState::LIFT_UP_UNLOADED;
            notifyStateChange();
        }
        break;
    case LiftState::MOVING_DOWN:
        // When stepper stops moving, return to appropriate idle state
        if (_stepper && !_stepper->isMoving())
        {
            MLOG_INFO("Lift [%s]: Movement complete", getId().c_str());
            liftState = _isLoaded ? LiftState::LIFT_DOWN_LOADED : LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }

        else if (_limitSwitch && _limitSwitch->isPressed() && liftState == LiftState::MOVING_DOWN)
        {
            MLOG_WARN("Lift [%s]: Limit switch triggered during downward movement - stopping", getId().c_str());
            _stepper->setCurrentPosition(0); // Reset position to zero
            liftState = LiftState::LIFT_DOWN_UNLOADED;
            notifyStateChange();
        }
        break;
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in loop()", getId().c_str());
    }
}

bool Lift::up(float speedRatio)
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    switch (liftState)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    case LiftState::LIFT_UP_UNLOADING:
        MLOG_WARN("Lift [%s]: Cannot move up, state is %s", getId().c_str(), stateToString(liftState).c_str());
        return false;

    case LiftState::LIFT_DOWN_UNLOADED:
    case LiftState::LIFT_DOWN_LOADED:
    case LiftState::MOVING_DOWN:
    case LiftState::MOVING_UP: // for changed speed
    {
        // Check if lift is already at or above max position
        long currentPos = _stepper->getCurrentPosition();
        if (currentPos >= _direction * _maxSteps)
        {
            MLOG_WARN("Lift [%s]: Cannot move up - already at max position (current: %ld, max: %ld)", getId().c_str(), currentPos, _direction * _maxSteps);
            return false;
        }

        MLOG_INFO("Lift [%s]: Moving up to %ld steps", getId().c_str(), _direction * _maxSteps);
        liftState = LiftState::MOVING_UP;
        notifyStateChange();
        return _stepper->moveTo(_direction * _maxSteps, _stepper->_defaultSpeed * speedRatio);
    }
    default:
        MLOG_ERROR("Lift [%s]: Unknown state encountered in up()", getId().c_str());
        return false;
    }
}

bool Lift::down(float speedRatio)
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    switch (liftState)
    {
    case LiftState::UNKNOWN:
    case LiftState::RESET:
    case LiftState::ERROR:
    case LiftState::LIFT_DOWN_UNLOADED:
    case LiftState::LIFT_DOWN_LOADED:
    case LiftState::LIFT_DOWN_LOADING:
    case LiftState::LIFT_UP_UNLOADING:
        MLOG_WARN("Lift [%s]: Cannot move down, state is %s", getId().c_str(), stateToString(liftState).c_str());
        return false;

    case LiftState::LIFT_UP_UNLOADED:
    case LiftState::LIFT_UP_LOADED:
    case LiftState::MOVING_DOWN: // for changed speed
    case LiftState::MOVING_UP:
    {
        // Check if lift is already at or below min position
        long currentPos = _stepper->getCurrentPosition();
        if (currentPos <= _direction * _minSteps)
        {
            MLOG_WARN("Lift [%s]: Cannot move down - already at min position (current: %ld, min: %ld)", getId().c_str(), currentPos, _direction * _minSteps);
            return false;
        }

        MLOG_INFO("Lift [%s]: Moving down to %ld steps", getId().c_str(), _direction * _minSteps);
        liftState = LiftState::MOVING_DOWN;
        notifyStateChange();
        return _stepper->moveTo(_direction * _minSteps, _stepper->_defaultSpeed * speedRatio);
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

    MLOG_INFO("Lift [%s]: Starting reset sequence", getId().c_str());

    liftState = LiftState::RESET;
    _resetStep = 1; // unload end

    // Slowly close the gate
    _loader->setValue(0.0f, 3000);

    notifyStateChange();
    return true;
}

bool Lift::loadBallStart()
{
    if (!_loader)
    {
        MLOG_WARN("Lift [%s]: Lift Loader not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Loading ball...", getId().c_str());
    liftState = LiftState::LIFT_DOWN_LOADING;
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

    liftState = LiftState::LIFT_DOWN_LOADED;
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
    liftState = LiftState::LIFT_UP_UNLOADING;
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

    liftState = LiftState::LIFT_UP_UNLOADED;
    notifyStateChange();

    // Set unloader to 0 (fully closed)
    return _unloader->setValue(0.0f);
}

bool Lift::loadBall()
{
    switch (liftState)
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
        MLOG_WARN("Lift [%s]: Cannot load ball, state is %s", getId().c_str(), stateToString(liftState).c_str());
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
    switch (liftState)
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
        MLOG_WARN("Lift [%s]: Cannot unload ball, state is %s", getId().c_str(), stateToString(liftState).c_str());
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
        float speedRatio = 1.0f;
        if (payload && (*payload)["speedRatio"].is<float>())
        {
            speedRatio = (*payload)["speedRatio"];
        }
        return up(speedRatio);
    }
    else if (action == "down")
    {
        float speedRatio = 1.0f;
        if (payload && (*payload)["speedRatio"].is<float>())
        {
            speedRatio = (*payload)["speedRatio"];
        }
        return down(speedRatio);
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
    doc["state"] = stateToString(liftState);
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