/**
 * @file IControllable.h
 * @brief Interface struct for controllable devices
 * 
 * Defines a struct-based interface that all controllable devices must implement
 * to provide a unified control mechanism across different hardware types.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef ICONTROLLABLE_H
#define ICONTROLLABLE_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @struct IControllable
 * @brief Interface struct for controllable devices
 * 
 * This struct defines the interface contract that all controllable devices
 * must implement. Uses pure virtual functions to enforce the interface.
 * In C++, structs can have virtual functions just like classes.
 */
struct IControllable {
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~IControllable() = default;
    
    /**
     * @brief Dynamic control function for device operations
     * @param action The action to perform (e.g., "set", "move", "toggle")
     * @param payload JSON object containing action parameters
     * @return true if action was successful, false otherwise
     */
    virtual bool control(const String& action, JsonObject& payload) = 0;
    
    /**
     * @brief Get device identifier
     * @return String identifier of the device
     */
    virtual String getId() const = 0;
};

#endif // ICONTROLLABLE_H
