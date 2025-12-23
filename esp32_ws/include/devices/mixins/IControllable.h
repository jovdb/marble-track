#ifndef I_CONTROLLABLE_H
#define I_CONTROLLABLE_H

#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>

class IControllable {
public:
    virtual ~IControllable() = default;
    virtual void addStateToJson(JsonDocument &doc) = 0;
    virtual bool control(const String &action, JsonObject *args = nullptr) = 0;
};

namespace mixins {
    class ControllableRegistry {
    public:
        static void registerDevice(const String &id, IControllable *ptr);
        static void unregisterDevice(const String &id);
        static IControllable *get(const String &id);
    };
}

#endif // I_CONTROLLABLE_H
