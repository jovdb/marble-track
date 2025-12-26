#include <ArduinoJson.h>
#include <vector>
#include "LittleFS.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/composition/Led.h"
#include "devices/composition/Button.h"

static constexpr const char *CONFIG_FILE = "/config.json";

void DeviceManager::loadDevicesFromJsonFile()
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        MLOG_INFO("File %s not found.", CONFIG_FILE);
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, FILE_READ);
    if (!file)
    {
        MLOG_ERROR("Failed to open config JSON file for reading");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err || !doc.is<JsonObject>())
    {
        MLOG_ERROR("Failed to parse config JSON file");
        return;
    }

    JsonObject rootObj = doc.as<JsonObject>();
    if (!rootObj["devices"].is<JsonArray>())
    {
        MLOG_INFO("No devices array found in config file");
        return;
    }

    JsonArray arr = rootObj["devices"];

    // Clear existing devices
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            delete devices[i];
            devices[i] = nullptr;
        }
    }
    devicesCount = 0;

    // Load devices from JSON
    for (JsonObject obj : arr)
    {
        const String id = obj["id"] | "";
        const String type = obj["type"] | "";

        if (id.isEmpty() || type.isEmpty())
        {
            MLOG_WARN("Skipping device with missing id or type");
            continue;
        }

        DeviceBase *newDevice = nullptr;

        // Create devices based on type
        if (type == "led")
        {
            newDevice = new composition::Led(id);
        }
        else if (type == "button")
        {
            newDevice = new composition::Button(id);
        }
        else
        {
            MLOG_WARN("Unknown device type: %s", type.c_str());
            continue;
        }

        if (newDevice)
        {
            // Load config if device is serializable and config exists
            if (newDevice->hasMixin("serializable") && obj["config"].is<JsonObject>())
            {
                JsonObject configObj = obj["config"];
                if (type == "led")
                {
                    composition::Led *ledDevice = static_cast<composition::Led *>(newDevice);
                    ledDevice->jsonToConfig(configObj);
                }
                else if (type == "button")
                {
                    composition::Button *buttonDevice = static_cast<composition::Button *>(newDevice);
                    buttonDevice->jsonToConfig(configObj);
                }
            }

            // Add to device manager
            addDevice(newDevice);

            MLOG_INFO("Loaded device: %s (%s)", id.c_str(), type.c_str());
        }
    }

    MLOG_INFO("Loaded %d devices from %s", devicesCount, CONFIG_FILE);
}

void DeviceManager::saveDevicesToJsonFile()
{
    // First, read the existing configuration to preserve other properties
    JsonDocument doc;
    bool fileExists = LittleFS.exists(CONFIG_FILE);

    if (fileExists)
    {
        File file = LittleFS.open(CONFIG_FILE, FILE_READ);
        if (file)
        {
            DeserializationError err = deserializeJson(doc, file);
            file.close();

            if (err)
            {
                MLOG_ERROR("Failed to parse existing configuration file, creating new one");
                doc.clear();
                doc.to<JsonObject>();
            }
        }
        else
        {
            MLOG_ERROR("Failed to read existing configuration file, creating new one");
            doc.clear();
            doc.to<JsonObject>();
        }
    }
    else
    {
        doc.to<JsonObject>();
    }

    // Ensure we have a root object
    if (!doc.is<JsonObject>())
    {
        doc.clear();
        doc.to<JsonObject>();
    }

    JsonObject rootObj = doc.as<JsonObject>();

    // Replace devices array with current devices snapshot
    rootObj.remove("devices");
    JsonArray devicesArray = rootObj["devices"].to<JsonArray>();

    for (int i = 0; i < devicesCount; i++)
    {
        DeviceBase *device = devices[i];
        if (!device)
            continue;

        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceObj["id"] = device->getId();
        deviceObj["type"] = device->getType();

        // Save children IDs
        JsonArray childrenArray = deviceObj["children"].to<JsonArray>();
        for (DeviceBase *child : device->getChildren())
        {
            childrenArray.add(child->getId());
        }

        // Only save config for devices that implement SerializableMixin
        if (device->hasMixin("serializable"))
        {
            if (device->getType() == "led")
            {
                composition::Led *ledDevice = static_cast<composition::Led *>(device);
                if (ledDevice)
                {
                    JsonDocument configDoc;
                    ledDevice->configToJson(configDoc);
                    deviceObj["config"] = configDoc.as<JsonObject>();
                }
            }
            else if (device->getType() == "button")
            {
                composition::Button *buttonDevice = static_cast<composition::Button *>(device);
                if (buttonDevice)
                {
                    JsonDocument configDoc;
                    buttonDevice->configToJson(configDoc);
                    deviceObj["config"] = configDoc.as<JsonObject>();
                }
            }
        }
    }

    // Save back to file
    File file = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (file)
    {
        serializeJson(doc, file);
        file.close();
        MLOG_INFO("Saved devices list to %s", CONFIG_FILE);
    }
    else
    {
        MLOG_ERROR("Failed to open %s for writing", CONFIG_FILE);
    }
}

NetworkSettings DeviceManager::loadNetworkSettings()
{
    NetworkSettings settings;

    if (!LittleFS.exists(CONFIG_FILE))
    {
        MLOG_INFO("Configuration file not found, could not connect to network");
        return settings;
    }

    File file = LittleFS.open(CONFIG_FILE, FILE_READ);
    if (file)
    {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        if (!err && doc.is<JsonObject>())
        {
            JsonObject rootObj = doc.as<JsonObject>();

            if (rootObj["network"].is<JsonObject>())
            {
                JsonObject networkObj = rootObj["network"];
                settings.ssid = networkObj["ssid"] | "";
                settings.password = networkObj["password"] | "";

                MLOG_INFO("Loaded network settings from config: SSID='%s'", settings.ssid.c_str());
            }
            else
            {
                MLOG_INFO("No network settings found in config file");
            }
        }
        else
        {
            MLOG_ERROR("Failed to parse configuration JSON file");
        }
    }
    else
    {
        MLOG_ERROR("Failed to open configuration file for reading");
    }

    return settings;
}

bool DeviceManager::saveNetworkSettings(const NetworkSettings &settings)
{
    JsonDocument doc;
    bool fileExists = LittleFS.exists(CONFIG_FILE);

    if (fileExists)
    {
        File file = LittleFS.open(CONFIG_FILE, FILE_READ);
        if (file)
        {
            DeserializationError err = deserializeJson(doc, file);
            file.close();

            if (err)
            {
                MLOG_ERROR("Failed to parse existing configuration file, creating new one");
                doc.clear();
                doc.to<JsonObject>();
            }
        }
        else
        {
            MLOG_ERROR("Failed to read existing configuration file, creating new one");
            doc.clear();
            doc.to<JsonObject>();
        }
    }
    else
    {
        doc.to<JsonObject>();
    }

    if (!doc.is<JsonObject>())
    {
        doc.clear();
        doc.to<JsonObject>();
    }

    JsonObject rootObj = doc.as<JsonObject>();

    JsonObject networkObj = rootObj["network"].to<JsonObject>();
    networkObj["ssid"] = settings.ssid;
    networkObj["password"] = settings.password;

    File file = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (file)
    {
        serializeJson(doc, file);
        file.close();
        MLOG_INFO("Saved network settings to config: SSID='%s'", settings.ssid.c_str());
        return true;
    }
    else
    {
        MLOG_ERROR("Failed to open configuration file for writing network settings");
        return false;
    }
}

DeviceManager::DeviceManager(NotifyClients callback) : devicesCount(0), notifyClients(callback), hasClients(nullptr)
{
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        devices[i] = nullptr;
    }
}

bool DeviceManager::addDevice(DeviceBase *device)
{
    if (devicesCount < MAX_DEVICES && device != nullptr)
    {
        devices[devicesCount] = device;
        devicesCount++;
        MLOG_INFO("Added device: %s (%s)", device->getId().c_str(), device->getType().c_str());
        return true;
    }

    if (device == nullptr)
    {
        MLOG_ERROR("Error: Cannot add null device");
    }
    else
    {
        MLOG_ERROR("Error: Device array is full, cannot add device: %s", device->getId().c_str());
    }
    return false;
}

bool DeviceManager::addDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    if (devicesCount >= MAX_DEVICES)
    {
        MLOG_ERROR("Cannot add device: Maximum device limit reached (%d)", MAX_DEVICES);
        return false;
    }

    if (getDeviceById(deviceId) != nullptr)
    {
        MLOG_ERROR("Cannot add device: Device with ID '%s' already exists", deviceId.c_str());
        return false;
    }

    DeviceBase *newDevice = nullptr;
    String lowerType = deviceType;
    lowerType.toLowerCase();

    if (lowerType == "led")
    {
        newDevice = new composition::Led(deviceId);
    }
    else if (lowerType == "button")
    {
        newDevice = new composition::Button(deviceId);
    }
    else
    {
        MLOG_ERROR("Unknown device type: %s", deviceType.c_str());
        return false;
    }

    if (newDevice)
    {
        // Load config if device is serializable and config exists
        if (newDevice->hasMixin("serializable") && config.is<JsonObject>())
        {
            JsonObject configObj = config.as<JsonObject>();
            if (lowerType == "led")
            {
                composition::Led *ledDevice = static_cast<composition::Led *>(newDevice);
                ledDevice->jsonToConfig(configObj);
            }
            else if (lowerType == "button")
            {
                composition::Button *buttonDevice = static_cast<composition::Button *>(newDevice);
                buttonDevice->jsonToConfig(configObj);
            }
        }

        devices[devicesCount] = newDevice;
        devicesCount++;

        MLOG_INFO("Added device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
        return true;
    }

    return false;
}

void DeviceManager::getDevices(DeviceBase **deviceList, int &count, int maxResults)
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

void DeviceManager::setup()
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            devices[i]->setup();
        }
    }
}

void DeviceManager::loop()
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            devices[i]->loop();
        }
    }
}

DeviceBase *DeviceManager::getDeviceById(const String &deviceId) const
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            DeviceBase *found = findDeviceRecursiveById(devices[i], deviceId);
            if (found)
                return found;
        }
    }
    return nullptr;
}

DeviceBase *DeviceManager::findDeviceRecursiveById(DeviceBase *root, const String &deviceId) const
{
    if (!root)
        return nullptr;
    if (root->getId() == deviceId)
        return root;
    for (DeviceBase *child : root->getChildren())
    {
        DeviceBase *found = findDeviceRecursiveById(child, deviceId);
        if (found)
            return found;
    }
    return nullptr;
}

DeviceBase *DeviceManager::getDeviceByType(const String &deviceType) const
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            DeviceBase *found = findDeviceRecursiveByType(devices[i], deviceType);
            if (found)
                return found;
        }
    }
    return nullptr;
}

DeviceBase *DeviceManager::findDeviceRecursiveByType(DeviceBase *root, const String &deviceType) const
{
    if (!root)
        return nullptr;
    if (root->getType() == deviceType)
        return root;
    for (DeviceBase *child : root->getChildren())
    {
        DeviceBase *found = findDeviceRecursiveByType(child, deviceType);
        if (found)
            return found;
    }
    return nullptr;
}

bool DeviceManager::removeDevice(const String &deviceId)
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr && devices[i]->getId() == deviceId)
        {
            MLOG_INFO("Removing device: %s (%s)", devices[i]->getId().c_str(), devices[i]->getType().c_str());
            delete devices[i];

            // Shift remaining devices down
            for (int j = i; j < devicesCount - 1; j++)
            {
                devices[j] = devices[j + 1];
            }
            devices[devicesCount - 1] = nullptr;
            devicesCount--;

            return true;
        }
    }

    MLOG_WARN("Device not found for removal: %s", deviceId.c_str());
    return false;
}

std::vector<DeviceBase *> DeviceManager::getAllDevices()
{
    std::vector<DeviceBase *> allDevices;

    std::function<void(DeviceBase *)> collectRecursive = [&](DeviceBase *device)
    {
        if (!device)
            return;
        allDevices.push_back(device);
        for (DeviceBase *child : device->getChildren())
        {
            collectRecursive(child);
        }
    };

    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            collectRecursive(devices[i]);
        }
    }

    return allDevices;
}
