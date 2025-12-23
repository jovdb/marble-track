/**
 * @file SimpleSensor.cpp
 * @brief Implementation of simple sensor using composition without RTOS
 */

#include "devices/composition/SimpleSensor.h"
#include "Logging.h"

SimpleSensor::SimpleSensor(const String &id)
    : DeviceBase(id, "sensor")
{
}

// =============================================================================
// DeviceBase overrides
// =============================================================================

void SimpleSensor::setup()
{
    if (_pin != -1)
    {
        pinMode(_pin, INPUT);
        MLOG_INFO("SimpleSensor [%s]: Setup on pin %d", _id.c_str(), _pin);
    }
    else
    {
        MLOG_WARN("SimpleSensor [%s]: Pin not configured", _id.c_str());
    }
}

void SimpleSensor::loop()
{
    if (_pin == -1)
        return;

    unsigned long now = millis();
    if (now - _lastPollTime < _pollInterval)
        return;

    _lastPollTime = now;

    int newValue = read();

    // Only notify if change exceeds threshold
    if (abs(newValue - _lastNotifiedValue) >= _threshold)
    {
        _lastNotifiedValue = newValue;
        notifyStateChange();
    }
}

std::vector<int> SimpleSensor::getPins() const
{
    if (_pin == -1)
        return {};
    return {_pin};
}

// =============================================================================
// SaveableMixin: config persistence
// =============================================================================

void SimpleSensor::loadConfigFromJson(const JsonDocument &config)
{
    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }

    if (config["threshold"].is<int>())
    {
        _threshold = config["threshold"].as<int>();
    }

    if (config["pollInterval"].is<unsigned long>())
    {
        _pollInterval = config["pollInterval"].as<unsigned long>();
    }
}

void SimpleSensor::saveConfigToJson(JsonDocument &doc) const
{
    doc["pin"] = _pin;
    doc["threshold"] = _threshold;
    doc["pollInterval"] = _pollInterval;
}

// =============================================================================
// ControllableMixin: control and state
// =============================================================================

bool SimpleSensor::handleControl(const String &action, JsonObject *args)
{
    if (action == "read")
    {
        read();
        notifyStateChange();
        return true;
    }

    if (action == "setThreshold")
    {
        if (!args || !(*args)["value"].is<int>())
        {
            MLOG_ERROR("SimpleSensor [%s]: Invalid 'setThreshold' payload", _id.c_str());
            return false;
        }
        _threshold = (*args)["value"].as<int>();
        notifyConfigChange();
        return true;
    }

    MLOG_WARN("SimpleSensor [%s]: Unknown action: %s", _id.c_str(), action.c_str());
    return false;
}

void SimpleSensor::addStateToJson(JsonDocument &doc) const
{
    doc["value"] = _value;
    doc["pin"] = _pin;
}

// =============================================================================
// Sensor-specific
// =============================================================================

int SimpleSensor::read()
{
    if (_pin == -1)
        return 0;

    _value = analogRead(_pin);
    return _value;
}
