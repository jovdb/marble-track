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

/**
 * @struct IDevice
 * @brief Base interface for all devices
 * 
 * This struct provides a common base interface for all devices in the system.
 * It allows for polymorphic device management and includes core functionality
 * like identification, naming, and loop operations.
 */
struct IDevice {
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
};

#endif // IDEVICE_H
