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

void ControllableTaskDevice::notifyClients(JsonDocument &doc)
{
    if (_notifyClients)
    {
        String message;
        serializeJson(doc, message);
        _notifyClients(message);
    }
    else
    {
        String message;
        serializeJson(doc, message);
        MLOG_WARN("%s: notifyClients callback is null, cannot send message: %s", toString().c_str(), message.c_str());
    }
}

void ControllableTaskDevice::notifyState(bool changed)
{
    JsonDocument doc;
    doc["type"] = "device-state";
    doc["deviceId"] = _id;
    // A UI can use get to load initial state and then just listen to changes
    doc["isChanged"] = changed;
    doc["state"] = getState();

    notifyClients(doc);
}

void ControllableTaskDevice::notifyConfig(bool changed)
{
    JsonDocument doc;
    doc["type"] = "device-config";
    doc["deviceId"] = _id;
    // A UI can use get to load initial state and then just listen to changes
    doc["isChanged"] = changed;
    doc["config"] = getConfig();

    MLOG_INFO("%s: Notifying config change for device", toString().c_str());
    notifyClients(doc);
}