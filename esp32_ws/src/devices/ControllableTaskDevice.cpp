#include "devices/ControllableTaskDevice.h"
#include "Logging.h"

ControllableTaskDevice::ControllableTaskDevice(const String &id, const String &type, NotifyClients callback)
    : SaveableTaskDevice(id, type), _notifyClients(callback)
{
}

ControllableTaskDevice::~ControllableTaskDevice()
{
    // Destructor implementation
}

JsonDocument ControllableTaskDevice::getState()
{
    // Default implementation: return basic state
    JsonDocument doc;
    doc["id"] = _id;
    doc["type"] = _type;
    addStateToJson(doc);
    return doc;
}

void ControllableTaskDevice::addStateToJson(JsonDocument &doc)
{
    // Default implementation: do nothing
}

bool ControllableTaskDevice::control(const String &action, JsonObject *args)
{
    // Default implementation: do nothing, return false
    return false;
}

std::vector<int> ControllableTaskDevice::getPins() const
{
    return std::vector<int>();
}

void ControllableTaskDevice::notifyStateChange()
{
    if (_notifyClients)
    {
        JsonDocument doc;
        doc["type"] = "device-state";
        doc["deviceId"] = _id;
        doc["state"] = getState();

        String message;
        serializeJson(doc, message);

        _notifyClients(message);
    }
    else
    {
        MLOG_WARN("No notifyClients callback set for device %s", toString().c_str());
    }
}

void ControllableTaskDevice::notifyConfigChange()
{
    if (_notifyClients)
    {
        JsonDocument doc;
        doc["type"] = "device-config";
        doc["deviceId"] = _id;
        doc["config"] = getConfig();

        String message;
        serializeJson(doc, message);

        MLOG_INFO("%s: Notifying config change for device: %s", toString().c_str(), message.c_str());
        _notifyClients(message);
    }
    else
    {
        MLOG_WARN("%s: No notifyClients callback set for device", toString().c_str());
    }
}