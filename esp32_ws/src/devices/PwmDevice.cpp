#include "devices/PwmDevice.h"
#include "Logging.h"

int PwmDevice::_nextChannel = 0;

PwmDevice::PwmDevice(const String &id, NotifyClients notifyClients)
    : Device(id, "pwm", notifyClients), _channel(_nextChannel++)
{
    if (_nextChannel >= 16)
    {
        _nextChannel = 0; // Wrap around, but risky
    }
}

PwmDevice::~PwmDevice()
{
    // Cleanup if needed
}

void PwmDevice::setup()
{
    if (_pin != -1)
    {
        ledcSetup(_channel, _frequency, _resolution);
        ledcAttachPin(_pin, _channel);
        ledcWrite(_channel, _dutyCycle);
    }
    else
    {
        MLOG_ERROR("PWM [%s]: Pin not configured", getId().c_str());
    }
}

void PwmDevice::loop()
{
    // Nothing to do in loop
}

bool PwmDevice::control(const String &action, JsonObject *payload)
{
    if (action == "set-duty-cycle")
    {
        if (payload && (*payload)["dutyCycle"].is<int>())
        {
            int duty = (*payload)["dutyCycle"].as<int>();
            return setDutyCycle(duty);
        }
        else
        {
            MLOG_WARN("PWM [%s]: set-duty-cycle requires dutyCycle in payload", getId().c_str());
            return false;
        }
    }
    else
    {
        MLOG_WARN("PWM [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
        return false;
    }
}

String PwmDevice::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    doc["dutyCycle"] = _dutyCycle;
    doc["frequency"] = _frequency;
    doc["resolution"] = _resolution;

    String result;
    serializeJson(doc, result);
    return result;
}

String PwmDevice::getConfig() const
{
    JsonDocument doc;
    // Copy base Device config fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getConfig());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    doc["pin"] = _pin;
    doc["frequency"] = _frequency;
    doc["resolution"] = _resolution;

    String result;
    serializeJson(doc, result);
    return result;
}

void PwmDevice::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("PWM [%s]: Null config provided", getId().c_str());
        return;
    }

    if ((*config)["pin"].is<int>())
    {
        _pin = (*config)["pin"].as<int>();
    }

    if ((*config)["frequency"].is<int>())
    {
        _frequency = (*config)["frequency"].as<int>();
    }

    if ((*config)["resolution"].is<int>())
    {
        _resolution = (*config)["resolution"].as<int>();
    }
}

std::vector<int> PwmDevice::getPins() const
{
    std::vector<int> pins;
    if (_pin != -1)
    {
        pins.push_back(_pin);
    }
    return pins;
}

bool PwmDevice::setDutyCycle(int dutyCycle)
{
    if (_pin == -1)
    {
        MLOG_WARN("PWM [%s]: Pin not configured", getId().c_str());
        return false;
    }

    _dutyCycle = dutyCycle;
    ledcWrite(_channel, _dutyCycle);
    notifyStateChange();
    MLOG_INFO("PWM [%s]: Set duty cycle to %d", getId().c_str(), _dutyCycle);
    return true;
}