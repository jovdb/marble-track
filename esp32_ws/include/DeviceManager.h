
#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <ArduinoJson.h>

#include "devices/Device.h"
#include <functional>
#include <Arduino.h>

using StateChangeCallback = std::function<void(const String &deviceId, const String &stateJson)>;

class DeviceManager
{

private:
    static const int MAX_DEVICES = 20;
    Device *devices[MAX_DEVICES];
    int devicesCount;

public:
    /**
     * @brief Recursively search for the first device of the given type
     * @param deviceType The type string to search for
     * @return Pointer to device or nullptr if not found
     */
    Device *getDeviceByType(const String &deviceType) const;

    /**
     * @brief Recursively search for the first device of the given type and cast to specific type
     * @tparam T Device type to cast to
     * @param deviceType The type string to search for
     * @return Pointer to device of type T or nullptr if not found or wrong type
     */
    template <typename T>
    T *getDeviceByTypeAs(const String &deviceType) const
    {
        return static_cast<T *>(getDeviceByType(deviceType));
    }

    /**
     * @brief Get device by ID
     * @param deviceId The ID of the device to find
     * @return Pointer to device or nullptr if not found
     */
    Device *getDeviceById(const String &deviceId) const;

    /**
     * @brief Get device by ID and cast to specific type
     * @tparam T Device type to cast to
     * @param deviceId The ID of the device to find
     * @return Pointer to device of type T or nullptr if not found or wrong type
     */
    template <typename T>
    T *getDeviceByIdAs(const String &deviceId) const
    {
        return static_cast<T *>(getDeviceById(deviceId));
    }
    /**
     * @brief Constructor - initializes empty device array
     */
    DeviceManager();

    /**
     * @brief Add a device to the management system
     * @param device Pointer to device to add
     * @return true if device was added successfully, false if array is full
     */
    bool addDevice(Device *device);

    /**
     * @brief Get all devices
     * @param deviceList Array to store pointers to devices
     * @param count Reference to store the number of devices found
     * @param maxResults Maximum number of results to return
     */
    void getDevices(Device **deviceList, int &count, int maxResults);

    /**
     * @brief Setup all devices and assign state change callback
     * @param callback The callback to assign to each device
     */
    void setup(StateChangeCallback callback);

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

        void loadDevicesFromJsonFile();
    void saveDevicesToJsonFile();
};

#endif // DEVICEMANAGER_H
