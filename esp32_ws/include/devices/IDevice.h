/**
 * @file IDevice.h
 * @brief Base interface for all devices
 *
 * Defines a base interface that all devices should implement to provide
 * a common base for polymorphic device management.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef IDEVICE_H
#define IDEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

// Callback function type for state change notifications
using StateChangeCallback = std::function<void(const String& deviceId, JsonObject state)>;

/**
 * @struct IDevice
 * @brief Base interface for all devices
 *
 * This struct provides a common base interface for all devices in the system.
 * It allows for polymorphic device management and includes core functionality
 * like identification, naming, and loop operations.
 */
struct IDevice
{
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~IDevice() = default;

    /**
     * @brief Get device identifier
     * @return String identifier of the device
     */
    virtual String getId() const = 0;

    /**
     * @brief Get device type, can be used for custom UI
     *
     */
    virtual String getType() const = 0;

    /**
     * @brief Get device name
     * @return String human-readable name of the device
     */
    virtual String getName() const = 0;

    /**
     * @brief Loop function for continuous device operations
     *
     * This function should be called repeatedly in the main loop to allow
     * the device to perform any periodic operations or state updates.
     */
    virtual void loop() = 0;

    /**
     * @brief Dynamic control function for device operations
     * @param action The action to perform (e.g., "set", "move", "toggle")
     * @param payload Pointer to JSON object containing action parameters (can be nullptr)
     * @return true if action was successful, false otherwise
     * @note Default implementation returns false (not controllable)
     */
    virtual bool control(const String& action, JsonObject* payload = nullptr) { return false; }
    
    /**
     * @brief Get current state of the device
     * @return JsonObject containing the current state of the device
     * @note Default implementation returns empty object (no state)
     */
    virtual JsonObject getState() { 
        JsonDocument doc;
        return doc.to<JsonObject>();
    }

    /**
     * @brief Check if this device supports remote control
     * @return true if device implements control functionality, false otherwise
     */
    virtual bool isControllable() const { return false; }

    /**
     * @brief Set callback function for state change notifications
     * @param callback Function to call when device state changes
     */
    virtual void setStateChangeCallback(StateChangeCallback callback) { stateChangeCallback = callback; }

protected:
    /**
     * @brief Callback function for state change notifications
     */
    StateChangeCallback stateChangeCallback = nullptr;

    /**
     * @brief Notify about state change if callback is set
     * @param state Current state of the device
     */
    void notifyStateChange(JsonObject state) {
        if (stateChangeCallback) {
            stateChangeCallback(getId(), state);
        }
    }
};

#endif // IDEVICE_H
