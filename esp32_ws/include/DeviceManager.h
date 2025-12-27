#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <ArduinoJson.h>
#include <functional>
#include <Arduino.h>
#include "NetworkSettings.h"
#include <vector>
#include "devices/composition/DeviceBase.h"

// Function types for WebSocket notifications
using NotifyClients = std::function<void(const String &message)>;
using HasClients = std::function<bool()>;

class DeviceManager
{

private:
    static const int MAX_DEVICES = 30;
    DeviceBase *devices[MAX_DEVICES];
    int devicesCount;

    NotifyClients notifyClients;
    HasClients hasClients;
    std::function<void()> onDevicesChanged;

public:
    NetworkSettings loadNetworkSettings();
    bool saveNetworkSettings(const NetworkSettings& settings);

    DeviceBase *getDeviceByType(const String &deviceType) const;

    template <typename T>
    T *getDeviceByTypeAs(const String &deviceType) const
    {
        return static_cast<T *>(getDeviceByType(deviceType));
    }

    DeviceBase *getDeviceById(const String &deviceId) const;

    template <typename T>
    T *getDeviceByIdAs(const String &deviceId) const
    {
        return static_cast<T *>(getDeviceById(deviceId));
    }

    DeviceManager(NotifyClients callback = nullptr);

    void setOnDevicesChanged(std::function<void()> callback) { onDevicesChanged = callback; }
    void setHasClients(HasClients callback) { hasClients = callback; }
    void notifyDevicesChanged() { if (onDevicesChanged) onDevicesChanged(); }

    bool addDevice(DeviceBase *device);
    bool removeDevice(const String &deviceId);
    bool addDevice(const String &deviceType, const String &deviceId, JsonVariant config = JsonVariant());

    /**
     * @brief Factory method to create a device based on type
     * @param deviceId The unique identifier for the device
     * @param deviceType The type of device to create ("led", "button", etc.)
     * @return Pointer to the created device, or nullptr if type is unknown
     */
    DeviceBase *createDevice(const String &deviceId, const String &deviceType);

    void getDevices(DeviceBase **deviceList, int &count, int maxResults);

    void setup();
    void loop();

    int getDeviceCount() const { return devicesCount; }

    std::vector<DeviceBase*> getAllDevices();

    void loadDevicesFromJsonFile();
    void saveDevicesToJsonFile();

    /**
     * @brief Populate a JSON array with a tree snapshot of all root devices
     *
     * The output uses a tree structure where:
     * - Only root devices are in the array
     * - Child devices are nested within their parents in a `children` array (as objects, not IDs)
     * - Each device object includes `id`, `type`, `children` (array of nested device objects)
     * - If a device implements the serializable mixin, a `config` object is included
     */
    void addDevicesToJsonArray(JsonArray &devicesArray);

private:
    /**
     * @brief Recursively apply config to devices from JSON
     * @param device The device to apply config to
     * @param deviceObj JSON object with optional children array and config
     */
    void loadDeviceConfigFromJson(DeviceBase *device, JsonObject deviceObj);

    /**
     * @brief Recursively add a device and its children to JSON array
     * @param device Device to serialize
     * @param deviceObj JSON object to populate
     */
    void addDeviceToJsonObject(DeviceBase *device, JsonObject deviceObj);

    DeviceBase *findDeviceRecursiveById(DeviceBase *root, const String &deviceId) const;
    DeviceBase *findDeviceRecursiveByType(DeviceBase *root, const String &deviceType) const;

    void deleteAllDevices();
};

#endif // DEVICEMANAGER_H
