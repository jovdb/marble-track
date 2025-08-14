/**
 * @file DeviceManager.cpp
 * @brief Implementation of device management system
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */


#include "DeviceManager.h"
#include "esp_log.h"

static const char *TAG = "DeviceManager";

DeviceManager::DeviceManager() : devicesCount(0)
{
    // Initialize device array to nullptr
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        devices[i] = nullptr;
    }
}

bool DeviceManager::addDevice(Device *device)
{
    if (devicesCount < MAX_DEVICES && device != nullptr)
    {
        devices[devicesCount] = device;
        devicesCount++;
    ESP_LOGI(TAG, "Added device: %s (%s)", device->getId().c_str(), device->getName().c_str());
        return true;
    }

    if (device == nullptr)
    {
    ESP_LOGE(TAG, "Error: Cannot add null device");
    }
    else
    {
    ESP_LOGE(TAG, "Error: Device array is full, cannot add device: %s", device->getId().c_str());
    }
    return false;
}

void DeviceManager::getDevices(Device **deviceList, int &count, int maxResults)
{
    count = 0;
    for (int i = 0; i < devicesCount && count < maxResults; i++)
    {
        if (devices[i] != nullptr)
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

Device *DeviceManager::getDeviceById(const String &deviceId) const
{
    // Helper function for recursive search
    std::function<Device *(Device *)> findRecursive = [&](Device *dev) -> Device *
    {
        if (!dev)
            return nullptr;
        if (dev->getId() == deviceId)
            return dev;
        for (Device *child : dev->getChildren())
        {
            Device *found = findRecursive(child);
            if (found)
                return found;
        }
        return nullptr;
    };
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            Device *found = findRecursive(devices[i]);
            if (found)
                return found;
        }
    }
    return nullptr;
}
