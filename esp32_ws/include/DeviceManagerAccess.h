/**
 * @file DeviceManagerAccess.h
 * @brief Provides access to the global DeviceManager instance
 *
 * This header provides external access to the DeviceManager instance
 * from main.cpp, allowing other modules to interact with devices.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef DEVICEMANAGERACCESS_H
#define DEVICEMANAGERACCESS_H

#include "DeviceManager.h"

/**
 * @brief Get access to the global device manager
 * @return Reference to the global DeviceManager instance
 * 
 * Use this function from other modules to access device management
 * functionality such as finding controllable devices by ID.
 * 
 * Example usage:
 * @code
 * DeviceManager& dm = getDeviceManager();
 * IControllable* device = dm.findControllableById("test-led");
 * if (device != nullptr) {
 *     JsonDocument payload;
 *     payload["state"] = true;
 *     device->control("set", &payload);
 * }
 * @endcode
 */
DeviceManager& getDeviceManager();

#endif // DEVICEMANAGERACCESS_H
