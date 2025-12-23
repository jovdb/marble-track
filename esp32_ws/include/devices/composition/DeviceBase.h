/**
 * @file DeviceBase.h
 * @brief Minimal base class for all devices using composition pattern
 *
 * This is the core device class that only provides:
 * - Identity (id, type, name)
 * - setup() and loop() lifecycle
 * - Children management
 *
 * All other functionality is added via composition mixins.
 */

#ifndef DEVICE_BASE_H
#define DEVICE_BASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @class DeviceBase
 * @brief Minimal base class for devices
 *
 * Provides only core identity and lifecycle. Extend with mixins for:
 * - RTOS task support (withRtos)
 * - Configuration persistence (makeSaveable)
 * - WebSocket control (makeControllable)
 */
class DeviceBase
{
public:
    DeviceBase(const String &id, const String &type);
    virtual ~DeviceBase() = default;

    // Lifecycle
    virtual void setup() {}
    virtual void loop() {}

    // Identity
    String getId() const { return _id; }
    String getType() const { return _type; }
    String getName() const { return _name; }
    void setName(const String &name) { _name = name; }

    // Hierarchy
    void addChild(DeviceBase *child);
    const std::vector<DeviceBase *> &getChildren() const { return _children; }

    // Pins (for collision detection)
    virtual std::vector<int> getPins() const { return {}; }

protected:
    String _id;
    String _type;
    String _name;
    std::vector<DeviceBase *> _children;
};

#endif // DEVICE_BASE_H
