#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <ArduinoJson.h>

#include "devices/Device.h"
#include "devices/TaskDevice.h"
#include "devices/ControllableTaskDevice.h"
#include <functional>
#include <Arduino.h>
#include "NetworkSettings.h"
#include <vector>
#include <map>
#include <set>
#include "devices/composition/DeviceBase.h"

class DeviceManager
{

private:
    static const int MAX_DEVICES = 20;
    Device *devices[MAX_DEVICES];
    int devicesCount;

    static const int MAX_TASK_DEVICES = 10;
    TaskDevice *taskDevices[MAX_TASK_DEVICES];
    int taskDevicesCount;

    // Composition devices (DeviceBase)
    static const int MAX_COMPOSITION_DEVICES = 20;
    DeviceBase *devices2[MAX_COMPOSITION_DEVICES];
    int devices2Count;

    NotifyClients notifyClients;
    HasClients hasClients;
    std::function<void()> onDevicesChanged;

public:
    /**
     * @brief Load network settings from the configuration file
     * @return NetworkSettings object with loaded settings, or empty settings if not found
     */
    NetworkSettings loadNetworkSettings();

    /**
     * @brief Save network settings to the configuration file
     * @param settings The network settings to save
     * @return true if saved successfully, false otherwise
     */
    bool saveNetworkSettings(const NetworkSettings& settings);

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
     * @brief Get task device by ID
     * @param deviceId The ID of the task device to find
     * @return Pointer to task device or nullptr if not found
     */
    TaskDevice *getTaskDeviceById(const String &deviceId) const;

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
     * @brief Get controllable task device by ID
     * @param deviceId The ID of the device to find
     * @return Pointer to ControllableTaskDevice or nullptr if not found
     */
    ControllableTaskDevice *getControllableTaskDeviceById(const String &deviceId) const;
    /**
     * @brief Constructor - initializes empty device array
     */
    DeviceManager(NotifyClients callback = nullptr);

    /**
     * @brief Set callback for when devices are added, removed, or config reloaded
     */
    void setOnDevicesChanged(std::function<void()> callback) { onDevicesChanged = callback; }

    /**
     * @brief Set hasClients callback
     */
    void setHasClients(HasClients callback) { hasClients = callback; }

    /**
     * @brief Notify that devices have changed
     */
    void notifyDevicesChanged() { if (onDevicesChanged) onDevicesChanged(); }

    /**
     * @brief Add a device to the management system
     * @param device Pointer to device to add
     * @return true if device was added successfully, false if array is full
     */
    bool addDevice(Device *device);

    /**
     * @brief Add a composition device (DeviceBase) to the management system
     * @param device Pointer to composition device to add
     * @return true if device was added successfully, false if array is full
     */
    bool addDevice(DeviceBase *device);

    /**
     * @brief Add a task device to the management system
     * @param device Pointer to task device to add
     * @return true if device was added successfully, false if array is full
     */
    bool addTaskDevice(TaskDevice *device);

    /**
     * @brief Remove a device from the management system
     * @param deviceId The ID of the device to remove
     * @return true if device was found and removed successfully, false otherwise
     */
    bool removeDevice(const String &deviceId);

    /**
     * @brief Create a new device of the specified type
     * @param deviceType The type of device to create (led, button, pwmmotor, etc.)
     * @param deviceId The unique ID for the new device
     * @param config Optional JSON configuration object
     * @param notifyCallback Optional callback for notifying clients
     * @param hasClientsCallback Optional callback to check if clients are connected
     * @return Pointer to the created device, or nullptr if creation failed
     */
    Device *createDevice(const String &deviceType, const String &deviceId, JsonVariant config = JsonVariant(), NotifyClients notifyCallback = nullptr, HasClients hasClientsCallback = nullptr);

    /**
     * @brief Create and add a new device to the management system
     * @param deviceType The type of device to create (led, button, pwmmotor, etc.)
     * @param deviceId The unique ID for the new device
     * @param config Optional JSON configuration object
     * @return true if device was created and added successfully, false otherwise
     */
    bool addDevice(const String &deviceType, const String &deviceId, JsonVariant config = JsonVariant());

    /**
     * @brief Get all devices
     * @param deviceList Array to store pointers to devices
     * @param count Reference to store the number of devices found
     * @param maxResults Maximum number of results to return
     */
    void getDevices(Device **deviceList, int &count, int maxResults);

    /**
     * @brief Get all task devices
     * @param deviceList Array to store pointers to task devices
     * @param count Reference to store the number of devices found
     * @param maxResults Maximum number of results to return
     */
    void getTaskDevices(TaskDevice **deviceList, int &count, int maxResults);

    /**
     * @brief Get all composition devices
     * @param deviceList Array to store pointers to devices
     * @param count Reference to store the number of devices found
     * @param maxResults Maximum number of results to return
     */
    void getCompositionDevices(DeviceBase **deviceList, int &count, int maxResults);

    /**
     * @brief Setup all devices and assign state change callback
     */
    void setup();

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
     * @brief Get all devices recursively (including children)
     * @return Vector of all devices
     */
    std::vector<Device*> getAllDevices();

    /**
     * @brief Get composition device by ID
     * @param deviceId The ID of the device to find
     * @return Pointer to composition device or nullptr if not found
     */
    DeviceBase *getCompositionDeviceById(const String &deviceId) const;

    /**
     * @brief Get composition device by ID and cast to specific type
     */
    template <typename T>
    T *getCompositionDeviceByIdAs(const String &deviceId) const
    {
        return static_cast<T *>(getCompositionDeviceById(deviceId));
    }

    void loadDevicesFromJsonFile();
    void saveDevicesToJsonFile();

private:
    JsonArray getDevicesFromJsonFile(JsonDocument& doc);
    void clearCurrentDevices();
    void createDevicesFromArray(JsonArray arr, std::map<String, Device*>& loadedDevices);
    void collectChildIds(JsonArray arr, std::set<String>& childIds);
    void addTopLevelDevices(std::map<String, Device*>& loadedDevices, std::set<String>& childIds);
    void linkChildren(JsonArray arr, std::map<String, Device*>& loadedDevices);
    DeviceBase *findCompositionDeviceRecursiveById(DeviceBase *root, const String &deviceId) const;
};

#endif // DEVICEMANAGER_H
