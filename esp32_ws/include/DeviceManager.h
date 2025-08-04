/**
 * @file DeviceManager.h
 * @brief Device management system for marble track controller
 *
 * This class manages all devices in the system, providing centralized
 * device registration, lookup, and lifecycle management.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "devices/IDevice.h"
#include <Arduino.h>

class DeviceManager
{
private:
    static const int MAX_DEVICES = 20;
    IDevice *devices[MAX_DEVICES];
    int devicesCount;

public:
    /**
     * @brief Constructor - initializes empty device array
     */
    DeviceManager();

    /**
     * @brief Add a device to the management system
     * @param device Pointer to device to add
     * @return true if device was added successfully, false if array is full
     */
    bool addDevice(IDevice *device);

    /**
     * @brief Get a controllable device by ID
     * @param deviceId The ID of the device to find
     * @return Pointer to IDevice or nullptr if not found/not controllable
     */
    IDevice* getControllableById(const String& deviceId);

    /**
     * @brief Get all controllable devices
     * @param deviceList Array to store pointers to controllable devices
     * @param count Reference to store the number of devices found
     * @param maxResults Maximum number of results to return
     */
    void getDevices(IDevice **deviceList, int &count, int maxResults);

    /**
     * @brief Call loop() function on all registered devices
     * This should be called from the main loop to update all devices
     */
    void loop();

    /**
     * @brief Get the total number of registered devices
     * @return Number of devices in the system
     */
    int getDeviceCount() const { return devicesCount; }

    /**
     * @brief Get the number of controllable devices
     * @return Number of devices that support control functionality
     */
    int getControllableCount() const;

    /**
     * @brief Get device by ID
     * @param deviceId The ID of the device to find
     * @return Pointer to device or nullptr if not found
     */
    IDevice *getDeviceById(const String &deviceId) const;
};

#endif // DEVICEMANAGER_H
