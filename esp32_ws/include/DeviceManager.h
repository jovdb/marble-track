#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <ArduinoJson.h>
#include <functional>
#include <Arduino.h>
#include "NetworkSettings.h"
#include <vector>
#include "devices/Device.h"

// Function types for WebSocket notifications
using NotifyClients = std::function<void(const String &message)>;
using HasClients = std::function<bool()>;

class DeviceManager
{

private:
    // Fixed-size array instead of std::vector to avoid heap fragmentation on
    // the ESP32's limited SRAM. Heap allocation during runtime can cause
    // long-term fragmentation that leads to allocation failures. The array
    // holds *root* devices only; each device's children are reached
    // recursively via Device::getChildren(). Raise MAX_DEVICES if the limit
    // is hit (currently 30 root slots).
    static const int MAX_DEVICES = 30;
    Device *devices[MAX_DEVICES];
    int devicesCount;

    NotifyClients notifyClients;
    HasClients hasClients;
    std::function<void()> onDevicesChanged;
    std::vector<std::function<void()>> onDevicesChangedListeners;

public:
    NetworkSettings loadNetworkSettings();
    bool saveNetworkSettings(const NetworkSettings& settings);
    bool loadLoggingSettings();
    bool saveLoggingSettings();

    Device *getDeviceByType(const String &deviceType) const;

    template <typename T>
    T *getDeviceByTypeAs(const String &deviceType) const
    {
        return static_cast<T *>(getDeviceByType(deviceType));
    }

    Device *getDeviceById(const String &deviceId) const;

    template <typename T>
    T *getDeviceByIdAs(const String &deviceId) const
    {
        return static_cast<T *>(getDeviceById(deviceId));
    }

    DeviceManager(NotifyClients callback = nullptr);

    // Subscribe to device-tree change events. Multiple subscribers are supported;
    // all are invoked from notifyDevicesChanged(). Prefer addOnDevicesChanged()
    // over setOnDevicesChanged() to avoid clobbering existing subscribers.
    void addOnDevicesChanged(std::function<void()> callback) { onDevicesChangedListeners.push_back(std::move(callback)); }
    void setOnDevicesChanged(std::function<void()> callback) { onDevicesChanged = callback; }
    void setHasClients(HasClients callback) { hasClients = callback; }
    void notifyDevicesChanged()
    {
        if (onDevicesChanged) onDevicesChanged();
        for (auto &cb : onDevicesChangedListeners) { if (cb) cb(); }
    }

    bool addDevice(Device *device);
    bool removeDevice(const String &deviceId);
    bool reorderDevices(const std::vector<String> &deviceIds);
    bool addDevice(const String &deviceType, const String &deviceId, JsonVariant config = JsonVariant());

    /**
     * @brief Factory method to create a device based on type
     * @param deviceId The unique identifier for the device
     * @param deviceType The type of device to create ("led", "button", etc.)
     * @return Pointer to the created device, or nullptr if type is unknown
     */
    Device *createDevice(const String &deviceId, const String &deviceType);

    void getDevices(Device **deviceList, int &count, int maxResults);

    void setup();
    void teardown();
    void loop();

    int getDeviceCount() const { return devicesCount; }

    std::vector<Device*> getAllDevices();

    void loadDevicesFromJsonFile();
    void saveDevicesToJsonFile();

    /**
     * @brief Atomically reload the device tree from /config.json.
     *
     * Performs the full safe reload sequence in one call:
     *   1. teardown()  \u2014 release hardware resources (LEDC, MCPWM, I2C, IO\u2011expander
     *                    state, stepper engines, etc.) so they are free before the
     *                    new tree tries to acquire them.
     *   2. loadDevicesFromJsonFile()  \u2014 drop existing devices and rebuild from disk.
     *   3. setup()     \u2014 initialize the new device tree.
     *   4. notifyDevicesChanged()  \u2014 fire listeners (CachedDeviceRef, website, ...).
     *
     * Callers (e.g. WebSocket set-devices-config handler) should prefer this over
     * calling the individual steps; getting the order wrong leaves hardware in a\n     * conflicting state and dangles cached device pointers.
     */
    void reloadFromJsonFile();

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
    void loadDeviceConfigFromJson(Device *device, JsonObject deviceObj);

    /**
     * @brief Recursively add a device and its children to JSON array
     * @param device Device to serialize
     * @param deviceObj JSON object to populate
     */
    void addDeviceToJsonObject(Device *device, JsonObject deviceObj);

    Device *findDeviceRecursiveById(Device *root, const String &deviceId) const;
    Device *findDeviceRecursiveByType(Device *root, const String &deviceType) const;

    void deleteAllDevices();
};

#endif // DEVICEMANAGER_H
