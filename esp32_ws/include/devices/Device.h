/**
 * @file Device.h
 * @brief Base interface for all devices
 *
 * Defines a base interface that all devices should implement to provide
 * a common base for polymorphic device management.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

// Callback function type for state change notifications
using StateChangeCallback = std::function<void(const String &deviceId, const String &stateJson)>;

/**
 * @class Device
 * @brief Base interface for all devices
 *
 * This class provides a common base interface for all devices in the system.
 * It allows for polymorphic device management and includes core functionality
 * like identification, naming, and loop operations.
 */
class Device
{
public:
    Device(const String &id, const String &name, const String &type);
    virtual void setup();
    virtual ~Device();
    void addChild(Device *child);
    std::vector<Device *> getChildren() const;
    virtual String getId() const { return _id; }
    virtual String getType() const { return _type; }
    virtual String getName() const { return _name; }
    virtual void loop();
    virtual bool control(const String &action, JsonObject *payload = nullptr);
    virtual String getState();
    virtual std::vector<int> getPins() const;
    virtual void setStateChangeCallback(StateChangeCallback callback);

    // Save a JSON object for a device id in a single config file
    bool saveConfig(const String &id, const JsonObject &json);
    // Read a config object for a device id from a single config file
    JsonDocument readConfig(const String &id);

protected:
    String _id;
    String _name;
    String _type;
    std::vector<Device *> children;
    /**
     * @brief Callback function for state change notifications
     */
    StateChangeCallback stateChangeCallback = nullptr;

    /**
     * @brief Notify about state change if callback is set
     * @param state Current state of the device
     */
    void notifyStateChange();
};

#endif // DEVICE_H
