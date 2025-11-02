#include "devices/Lift.h"
#include "Logging.h"

Lift::Lift(const String &id, NotifyClients notifyClients)
    : Device(id, "lift", notifyClients), _stepper(nullptr), _sensor(nullptr), _state(liftState::IDLE)
{
}

void Lift::setup()
{
    auto children = getChildren();
    if (children.empty())
    {
        // Create children if not loaded from config
        _stepper = new Stepper(getId() + "-stepper", _notifyClients);
        _sensor = new Button(getId() + "-limit", _notifyClients);
        addChild(_stepper);
        addChild(_sensor);
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
}

void Lift::loop()
{
    Device::loop();

    if (!_stepper)
        return;

    if (_state == liftState::MOVING && !_stepper->isMoving())
    {
        _state = liftState::IDLE;
        notifyStateChange();
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
    _state = liftState::MOVING;
    notifyStateChange();
    return _stepper->moveTo(_maxSteps);
}

bool Lift::down()
{
    if (!_stepper)
    {
        MLOG_WARN("Lift [%s]: Stepper not initialized", getId().c_str());
        return false;
    }

    MLOG_INFO("Lift [%s]: Moving down to %ld steps", getId().c_str(), _minSteps);
    _state = liftState::MOVING;
    notifyStateChange();
    return _stepper->moveTo(_minSteps);
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
    case Lift::liftState::IDLE:
        return "IDLE";
    case Lift::liftState::MOVING:
        return "MOVING";
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