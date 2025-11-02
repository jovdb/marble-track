#include "devices/Lift.h"
#include "Logging.h"

Lift::Lift(const String &id, NotifyClients notifyClients)
    : Device(id, "lift", notifyClients), _stepper(nullptr), _limitSwitch(nullptr), _ballSensor(nullptr), _gate(nullptr), _state(liftState::UNKNOWN)
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
        _gate = new PwmMotor(getId() + "-gate", _notifyClients);
        addChild(_stepper);
        addChild(_limitSwitch);
        addChild(_ballSensor);
        addChild(_gate);
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
            _gate = static_cast<PwmMotor *>(children[3]);
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
    case liftState::UNKNOWN:
    {
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            _stepper->setCurrentPosition(0);
            _state = (_ballSensor && _ballSensor->isPressed()) ? liftState::BALL_WAITING : liftState::IDLE;
            notifyStateChange();
        }
        break;
    }
    case liftState::IDLE:
        if (_ballSensor && _ballSensor->isPressed())
        {
            MLOG_INFO("Lift [%s]: Ball loaded", getId().c_str());
            _state = liftState::BALL_WAITING;
            notifyStateChange();
        }
        break;
    case liftState::BALL_WAITING:
        if (_ballSensor && _ballSensor->onReleased())
        {
            MLOG_INFO("Lift [%s]: Ball unloaded", getId().c_str());
            _state = liftState::IDLE;
            notifyStateChange();
        }
        break;
    case liftState::RESET:
        // During reset, check if limit switch is pressed
        if (_limitSwitch && _limitSwitch->isPressed())
        {
            MLOG_INFO("Lift [%s]: Reset complete - limit switch pressed", getId().c_str());
            // _stepper->stop(100000); // Stop the stepper
            _stepper->setCurrentPosition(0); // Reset position to zero
            _state = (_ballSensor && _ballSensor->isPressed()) ? liftState::BALL_WAITING : liftState::IDLE;
            notifyStateChange();
        }
        else if (_stepper && !_stepper->isMoving())
        {
            MLOG_WARN("Lift [%s]: Reset incomplete - Lift stopped but limit switch not pressed", getId().c_str());
            _state = liftState::ERROR;
            notifyStateChange();
        }
        break;
    case liftState::MOVING_UP:
    case liftState::MOVING_DOWN:
        // When stepper stops moving, return to appropriate idle state
        if (_stepper && !_stepper->isMoving())
        {
            MLOG_INFO("Lift [%s]: Movement complete", getId().c_str());
            _state = (_ballSensor && _ballSensor->isPressed()) ? liftState::BALL_WAITING : liftState::IDLE;
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

    MLOG_INFO("Lift [%s]: Moving up to %ld steps", getId().c_str(), _maxSteps);
    _state = liftState::MOVING_UP;
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

    MLOG_INFO("Lift [%s]: Moving down to %ld steps", getId().c_str(), _minSteps);
    _state = liftState::MOVING_DOWN;
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

        _state = liftState::IDLE;
        notifyStateChange();
        return true;
    }

    MLOG_INFO("Lift [%s]: Starting reset - moving down slowly until limit switch is pressed", getId().c_str());
    _state = liftState::RESET;
    notifyStateChange();

    // Move down slowly (negative direction) until limit switch is pressed
    // We'll move in small steps and check the limit switch
    return _stepper->move(_maxSteps - _minSteps, _stepper ? _stepper->_defaultSpeed / 2 : 100);
}

bool Lift::gateUp()
{
    if (!_gate)
    {
        MLOG_WARN("Lift [%s]: Gate not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Setting gate to up (100)", getId().c_str());
    _state = liftState::LIFT_LOADED;
    notifyStateChange();

    // Set gate to 100 (fully open)
    return _gate->setValue(100.0f);
}

bool Lift::gateDown()
{
    if (!_gate)
    {
        MLOG_WARN("Lift [%s]: Gate not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Setting gate to down (0)", getId().c_str());
    _state = liftState::IDLE;
    notifyStateChange();

    // Set gate to 0 (fully closed)
    return _gate->setValue(0.0f);
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
    else if (action == "gateUp")
    {
        return gateUp();
    }
    else if (action == "gateDown")
    {
        return gateDown();
    }
    else
    {
        MLOG_WARN("Lift [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
    }

    return false;
}

String Lift::stateToString(Lift::liftState state) const
{
    switch (state)
    {
    case Lift::liftState::UNKNOWN:
        return "Unknown";
    case Lift::liftState::IDLE:
        return "Idle";
    case Lift::liftState::BALL_WAITING:
        return "BallWaiting";
    case Lift::liftState::RESET:
        return "Reset";
    case Lift::liftState::ERROR:
        return "Error";
    case Lift::liftState::LIFT_LOADED:
        return "LiftLoaded";
    case Lift::liftState::MOVING_UP:
        return "MovingUp";
    case Lift::liftState::MOVING_DOWN:
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
        _name = (*config)["name"].as<String>();
    }

    // Set minSteps if provided
    if ((*config)["minSteps"].is<long>())
    {
        _minSteps = (*config)["minSteps"].as<long>();
    }

    // Set maxSteps if provided
    if ((*config)["maxSteps"].is<long>())
    {
        _maxSteps = (*config)["maxSteps"].as<long>();
    }
}