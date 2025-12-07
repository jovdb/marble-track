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
    // Should be overriden
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
    String message;
    serializeJson(doc, message);

    if (_notifyClients)
    {
        _notifyClients(message);
    }
    else
    {
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

    MLOG_INFO("%s: Notifying state %sfor device", toString().c_str(), changed ? "change " : "");
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

    MLOG_INFO("%s: Notifying config %sfor device", toString().c_str(), changed ? "change " : "");
    notifyClients(doc);
}

void ControllableTaskDevice::notifyError(const String &type, const String &error)
{
    JsonDocument doc;
    doc["type"] = type;
    doc["deviceId"] = _id;
    doc["error"] = error;

    MLOG_ERROR("%s: Notifying error for device: %s", toString().c_str(), error.c_str());
    notifyClients(doc);
}