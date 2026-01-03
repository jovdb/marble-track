/**
 * @file Device.h
 * @brief Minimal base class for all devices using composition pattern
 *
 * This is the core device class that only provides:
 * - Identity (id, type, name)
 * - setup() and loop() lifecycle
 * - Children management
 *
 * All other functionality is added via composition mixins.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @class Device
 * @brief Minimal base class for devices
 *
 * Provides only core identity and lifecycle. Extend with mixins for:
 * - RTOS task support (withRtos)
 * - Configuration persistence (makeSerializable)
 * - WebSocket control (makeControllable)
 */
class Device
{
public:
    Device(const String &id, const String &type);
    virtual ~Device() = default;

    // Lifecycle
    // Default setup calls setup on children
    virtual void setup();
    virtual void loop();

    // Identity
    String getId() const { return _id; }
    String getType() const { return _type; }
    String getName() const { return _name; }
    void setName(const String &name) { _name = name; }
    virtual String toString() const;

    // Hierarchy
    void addChild(Device *child);
    const std::vector<Device *> &getChildren() const { return _children; }
    Device *getChildById(const String &id) const;

    template <typename T>
    T *getChildByIdAs(const String &id) const
    {
        return static_cast<T *>(getChildById(id));
    }

    // Pins (for collision detection)
    virtual std::vector<String> getPins() const { return {}; }

    // Mixin detection
    bool hasMixin(const String &mixinName) const;
    const std::vector<String> &getMixins() const { return _mixins; }
    void registerMixin(const String &mixinName);

protected:
    String _id;
    String _type;
    String _name;
    bool _isInitialized = false;
    std::vector<Device *> _children;
    std::vector<String> _mixins;
};

#endif // DEVICE_H
