/**
 * @file ControllableMixin.h
 * @brief WebSocket control and state notification mixin
 *
 * Adds control (from WS messages) and state notification capabilities.
 * The derived class must implement:
 * - bool handleControl(const String &action, JsonObject *args)
 * - void addStateToJson(JsonDocument &doc) const
 *
 * Usage:
 *   class MyDevice : public DeviceBase, public ControllableMixin<MyDevice> {
 *       bool handleControl(const String &action, JsonObject *args) override { ... }
 *       void addStateToJson(JsonDocument &doc) const override { ... }
 *   };
 */

#ifndef CONTROLLABLE_MIXIN_H
#define CONTROLLABLE_MIXIN_H

#include <ArduinoJson.h>
#include <Arduino.h>
#include <functional>

using NotifyClientsCallback = std::function<void(const String &message)>;

/**
 * @class ControllableMixin
 * @brief Mixin that adds WS control and state notifications
 * @tparam Derived The derived class (CRTP pattern)
 */
template <typename Derived>
class ControllableMixin
{
public:
    virtual ~ControllableMixin() = default;

    /**
     * @brief Set the callback for notifying WebSocket clients
     * @param callback Function to call with JSON message strings
     */
    void setNotifyClientsCallback(NotifyClientsCallback callback)
    {
        _notifyClients = callback;
    }

    /**
     * @brief Control the device with an action
     * @param action The action to perform
     * @param args Optional JSON object with arguments
     * @return true if action was successful
     */
    bool control(const String &action, JsonObject *args = nullptr)
    {
        return static_cast<Derived *>(this)->handleControl(action, args);
    }

    /**
     * @brief Get current state as JSON document
     * @return JsonDocument with id, type, and device-specific state
     */
    JsonDocument getState() const
    {
        JsonDocument doc;
        const auto *derived = static_cast<const Derived *>(this);

        // Add base identity
        doc["id"] = derived->getId();
        doc["type"] = derived->getType();

        // Let derived class add its specific state
        derived->addStateToJson(doc);

        return doc;
    }

    /**
     * @brief Notify clients about state change
     * @param changed Whether the state actually changed (default: true)
     */
    void notifyStateChange(bool changed = true)
    {
        if (!_notifyClients)
            return;

        const auto *derived = static_cast<const Derived *>(this);

        JsonDocument doc;
        doc["type"] = "device-state";
        doc["deviceId"] = derived->getId();
        doc["isChanged"] = changed;
        doc["state"] = getState();

        String message;
        serializeJson(doc, message);
        _notifyClients(message);
    }

    /**
     * @brief Notify clients about config change
     * @param changed Whether the config actually changed
     *
     * Note: This method requires the derived class to also inherit from
     * SaveableMixin (or provide getConfig()). Call notifyConfigChangeWithConfig()
     * if you want to provide the config explicitly.
     */
    void notifyConfigChange(bool changed = true)
    {
        if (!_notifyClients)
            return;

        const auto *derived = static_cast<const Derived *>(this);

        JsonDocument doc;
        doc["type"] = "device-config";
        doc["deviceId"] = derived->getId();
        doc["isChanged"] = changed;

        // Call getConfig() on derived - requires SaveableMixin or equivalent
        doc["config"] = derived->getConfig();

        String message;
        serializeJson(doc, message);
        _notifyClients(message);
    }

    /**
     * @brief Notify clients about config change with explicit config
     * @param config The config document to send
     * @param changed Whether the config actually changed
     */
    void notifyConfigChangeWithConfig(const JsonDocument &config, bool changed = true)
    {
        if (!_notifyClients)
            return;

        const auto *derived = static_cast<const Derived *>(this);

        JsonDocument doc;
        doc["type"] = "device-config";
        doc["deviceId"] = derived->getId();
        doc["isChanged"] = changed;
        doc["config"] = config;

        String message;
        serializeJson(doc, message);
        _notifyClients(message);
    }

    /**
     * @brief Notify clients about an error
     * @param errorType Type of error message
     * @param error Error message
     */
    void notifyError(const String &errorType, const String &error)
    {
        if (!_notifyClients)
            return;

        const auto *derived = static_cast<const Derived *>(this);

        JsonDocument doc;
        doc["type"] = errorType;
        doc["deviceId"] = derived->getId();
        doc["error"] = error;

        String message;
        serializeJson(doc, message);
        _notifyClients(message);
    }

    /**
     * @brief Check if this device is controllable
     * @return true (always for ControllableMixin)
     */
    bool isControllable() const { return true; }

protected:
    NotifyClientsCallback _notifyClients = nullptr;

    /**
     * @brief Handle a control action
     * Must be implemented by derived class.
     * @param action The action to perform
     * @param args Optional JSON arguments
     * @return true if action was handled successfully
     */
    virtual bool handleControl(const String &action, JsonObject *args) = 0;

    /**
     * @brief Add device-specific state to JSON
     * Must be implemented by derived class.
     * @param doc The JSON document to extend
     */
    virtual void addStateToJson(JsonDocument &doc) const = 0;
};

#endif // CONTROLLABLE_MIXIN_H
