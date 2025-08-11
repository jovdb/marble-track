#include "devices/Device.h"

Device::Device(const String &name, const String &type) : _name(name), _type(type) {}

Device::~Device()
{
    for (Device *child : children)
    {
        delete child;
    }
}

void Device::addChild(Device *child)
{
    if (child)
    {
        children.push_back(child);
    }
}

void Device::setup()
{
    for (Device *child : children)
    {
        if (child)
            child->setup();
    }
}

void Device::loop()
{
    for (Device *child : children)
    {
        if (child)
            child->loop();
    }
}

bool Device::control(const String &action, JsonObject *payload)
{
    return false;
}

String Device::getState()
{
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;

    // If there are children, add their states to a 'children' array
    if (!children.empty()) {
        JsonArray childrenArr = doc.createNestedArray("children");
        for (Device *child : children) {
            if (child) {
                String childStateStr = child->getState();
                JsonDocument childDoc;
                deserializeJson(childDoc, childStateStr);
                childrenArr.add(childDoc.as<JsonObject>());
            }
        }
    }

    String result;
    serializeJson(doc, result);
    return result;
}

std::vector<int> Device::getPins() const
{
    std::vector<int> pins;
    for (const Device *child : children)
    {
        if (child)
        {
            auto childPins = child->getPins();
            pins.insert(pins.end(), childPins.begin(), childPins.end());
        }
    }
    return pins;
}

void Device::setStateChangeCallback(StateChangeCallback callback)
{
    stateChangeCallback = callback;
    for (Device *child : children)
    {
        if (child)
            child->setStateChangeCallback(callback);
    }
}

void Device::notifyStateChange()
{
    if (stateChangeCallback)
    {
        stateChangeCallback(getId(), getState());
    }
}
