
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "devices/Device.h"
#include "devices/Device.h"
#include <vector>
#include "Logging.h"

Device::Device(const String &id, const String &type, NotifyClients callback) : _id(id), _name(""), _type(type), notifyClients(callback) {}

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
    /*
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
  */

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

void Device::notifyStateChange()
{
    if (notifyClients)
    {
        JsonDocument doc;
        doc["type"] = "device-state";
        doc["deviceId"] = this->getId();

        String stateJson = this->getState();
        JsonDocument stateDoc;
        if (deserializeJson(stateDoc, stateJson) == DeserializationError::Ok)
        {
            doc["state"] = stateDoc.as<JsonObject>();
        }
        else
        {
            // Fallback: add the string directly if parsing fails
            doc["state"] = stateJson;
        }
        /*
        if (!error.isEmpty())
            doc["error"] = error;
        */
        String message;
        serializeJson(doc, message);

        notifyClients(message);
    }
    else
    {
        MLOG_WARN("Device [%s]: notifyClients callback not set", _id.c_str());
    }
}

void Device::notifyError(String messageType, String error)
{
    if (notifyClients)
    {
        JsonDocument doc;
        doc["type"] = messageType;
        doc["deviceId"] = this->getId();
        doc["error"] = error;

        String message;
        serializeJson(doc, message);

        notifyClients(message);
    }
    else
    {
        MLOG_WARN("Device [%s]: notifyClients callback not set", _id.c_str());
    }
}

String Device::getConfig() const
{
    // Default implementation: return empty JSON object
    return "{}";
}

void Device::setConfig(JsonObject *config)
{
    // Default implementation: do nothing
    // Derived classes can override to handle config
}