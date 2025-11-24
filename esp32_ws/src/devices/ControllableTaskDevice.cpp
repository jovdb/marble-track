#include "devices/ControllableTaskDevice.h"

ControllableTaskDevice::ControllableTaskDevice(const String &id, const String &type)
    : SaveableTaskDevice(id, type)
{
}

ControllableTaskDevice::~ControllableTaskDevice()
{
    // Destructor implementation
}

String ControllableTaskDevice::getState()
{
    // Default implementation: return basic state
    JsonDocument doc;
    doc["id"] = _id;
    doc["type"] = _type;
    addStateToJson(doc);

    String state;
    serializeJson(doc, state);
    return state;
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