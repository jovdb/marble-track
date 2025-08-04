/**
 * @file DeviceManager.cpp
 * @brief Implementation of device management system
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "DeviceManager.h"

DeviceManager::DeviceManager() : devicesCount(0)
{
    // Initialize device array to nullptr
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        devices[i] = nullptr;
    }
}

bool DeviceManager::addDevice(IDevice* device)
{
    if (devicesCount < MAX_DEVICES && device != nullptr)
    {
        devices[devicesCount] = device;
        devicesCount++;
        Serial.println("Added device: " + device->getId() + " (" + device->getName() + ")");
        return true;
    }
    
    if (device == nullptr)
    {
        Serial.println("Error: Cannot add null device");
    }
    else
    {
        Serial.println("Error: Device array is full, cannot add device: " + device->getId());
    }
    return false;
}

IDevice* DeviceManager::getControllableById(const String& deviceId)
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr && devices[i]->getId() == deviceId)
        {
            // Check if device supports controllable functionality
            if (devices[i]->isControllable())
            {
                return devices[i];
            }
        }
    }
    return nullptr;
}

void DeviceManager::getDevices(IDevice** deviceList, int& count, int maxResults)
{
    count = 0;
    for (int i = 0; i < devicesCount && count < maxResults; i++)
    {
        if (devices[i] != nullptr && devices[i]->isControllable())
        {
            deviceList[count] = devices[i];
            count++;
        }
    }
}

void DeviceManager::loop()
{
    // Call loop() on all registered devices
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            devices[i]->loop();
        }
    }
}

int DeviceManager::getControllableCount() const
{
    int deviceCount = 0;
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr && devices[i]->isControllable())
        {
            deviceCount++;
        }
    }
    return deviceCount;
}

IDevice* DeviceManager::getDeviceById(const String& deviceId) const
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr && devices[i]->getId() == deviceId)
        {
            return devices[i];
        }
    }
    return nullptr;
}
