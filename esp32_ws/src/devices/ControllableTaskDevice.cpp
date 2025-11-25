#include "devices/ControllableTaskDevice.h"

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

        _notifyClients(message);
    }
}