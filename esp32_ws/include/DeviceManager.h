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
    static const int MAX_DEVICES = 20;
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

    void getDevices(DeviceBase **deviceList, int &count, int maxResults);

    void setup();
    void loop();

    int getDeviceCount() const { return devicesCount; }

    std::vector<DeviceBase*> getAllDevices();

    void loadDevicesFromJsonFile();
    void saveDevicesToJsonFile();

private:
    DeviceBase *findDeviceRecursiveById(DeviceBase *root, const String &deviceId) const;
    DeviceBase *findDeviceRecursiveByType(DeviceBase *root, const String &deviceType) const;
};

#endif // DEVICEMANAGER_H
