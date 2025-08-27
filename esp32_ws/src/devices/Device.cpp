#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "devices/Device.h"
#include "devices/Device.h"
#include <vector>

Device::Device(const String &id, const String &name, const String &type) : _id(id), _name(name), _type(type) {}

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

std::vector<Device *> Device::getChildren() const
{
    return children;
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
    doc["id"] = _id;
    doc["id"] = _id;
    doc["type"] = _type;
    doc["name"] = _name;

    // If there are children, add their states to a 'children' array
    if (!children.empty())
    {
        JsonArray childrenArr = doc["children"].to<JsonArray>();
        for (Device *child : children)
        {
            if (child)
            {
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

bool Device::saveConfig(const String &id, const JsonObject &json)
{
    String filename = "/device_config.json";
    JsonDocument allConfig;
    // Read existing config file
    File file = SPIFFS.open(filename, FILE_READ);
    if (file)
    {
        deserializeJson(allConfig, file);
        file.close();
    }
    // Update config for this device id
    allConfig[id] = json;
    // Write back to file
    file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
        return false;
    serializeJson(allConfig, file);
    file.close();
    return true;
}

JsonDocument Device::readConfig(const String &id)
{
    String filename = "/device_config.json";
    JsonDocument allConfig;
    File file = SPIFFS.open(filename, FILE_READ);
    if (file)
    {
        deserializeJson(allConfig, file);
        file.close();
    }
    JsonDocument doc;
    if (allConfig[id].is<JsonObject>())
    {
        doc.set(allConfig[id]);
    }
    return doc;
}
