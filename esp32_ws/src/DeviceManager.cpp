#include <ArduinoJson.h>
#include <vector>
#include <map>
#include <set>
#include "LittleFS.h"
#include "Logging.h"
#include "TimeManager.h"
#include "DeviceManager.h"
#include "Network.h"
#include "WebSocketManager.h"
#include "devices/Led.h"
#include "devices/Buzzer.h"
#include "devices/Button.h"
#include "devices/Stepper.h"
#include "devices/Wheel.h"
#include "devices/PwmMotor.h"
#include "devices/Lift.h"
#include "devices/composition/Led.h"
#include "devices/composition/Button.h"
#include "devices/composition/Test2.h"
#include "devices/mixins/SerializableMixin.h"

static constexpr const char *CONFIG_FILE = "/config.json";

JsonArray DeviceManager::getDevicesFromJsonFile(JsonDocument& doc)
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        MLOG_INFO("File %s not found.", CONFIG_FILE);
        return JsonArray();
    }

    File file = LittleFS.open(CONFIG_FILE, FILE_READ);
    if (file)
    {
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        if (!err && doc.is<JsonObject>())
        {
            JsonObject rootObj = doc.as<JsonObject>();

            // Check if devices array exists
            if (rootObj["devices"].is<JsonArray>())
            {
                return rootObj["devices"];
            }
            else
            {
                MLOG_INFO("No devices array found in config file");
            }
        }
        else
        {
            MLOG_ERROR("Failed to parse config JSON file");
        }
    }
    else
    {
        MLOG_ERROR("Failed to open config JSON file for reading");
    }

    return JsonArray();
}

void DeviceManager::clearCurrentDevices()
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            delete devices[i];
            devices[i] = nullptr;
        }
    }
    devicesCount = 0;
}

void DeviceManager::createDevicesFromArray(JsonArray arr, std::map<String, Device*>& loadedDevices)
{
    for (JsonObject obj : arr)
    {
        const String id = obj["id"] | "";
        const String type = obj["type"] | "";

        if (loadedDevices.find(id) != loadedDevices.end())
        {
            MLOG_WARN("Duplicate device ID '%s' in config, skipping", id.c_str());
            continue;
        }

        Device *newDevice = createDevice(type, id, obj["config"], notifyClients, hasClients);
        if (newDevice == nullptr)
        {
            MLOG_WARN("Unknown device type: %s", type.c_str());
            continue;
        }
        loadedDevices[id] = newDevice;
    }
}

void DeviceManager::collectChildIds(JsonArray arr, std::set<String>& childIds)
{
    for (JsonObject obj : arr)
    {
        if (obj["children"].is<JsonArray>())
        {
            JsonArray childrenArr = obj["children"];
            for (String childId : childrenArr)
            {
                childIds.insert(childId);
            }
        }
    }
}

void DeviceManager::addTopLevelDevices(std::map<String, Device*>& loadedDevices, std::set<String>& childIds)
{
    for (auto &pair : loadedDevices)
    {
        if (childIds.find(pair.first) == childIds.end())
        {
            if (devicesCount < MAX_DEVICES)
            {
                devices[devicesCount++] = pair.second;
            }
            else
            {
                MLOG_WARN("Maximum device limit reached, cannot add top-level device: %s", pair.first.c_str());
                delete pair.second;
            }
        }
    }
}

void DeviceManager::linkChildren(JsonArray arr, std::map<String, Device*>& loadedDevices)
{
    for (JsonObject obj : arr)
    {
        const String id = obj["id"] | "";
        auto it = loadedDevices.find(id);
        if (it != loadedDevices.end() && obj["children"].is<JsonArray>())
        {
            Device *parent = it->second;
            JsonArray childrenArr = obj["children"];
            for (String childId : childrenArr)
            {
                auto childIt = loadedDevices.find(childId);
                if (childIt != loadedDevices.end())
                {
                    parent->addChild(childIt->second);
                }
                else
                {
                    MLOG_WARN("Child device '%s' not found for parent '%s'", childId.c_str(), id.c_str());
                }
            }
        }
    }
}

void DeviceManager::loadDevicesFromJsonFile()
{
    JsonDocument doc;
    JsonArray arr = getDevicesFromJsonFile(doc);

    if (arr.size() > 0)
    {
        // Clear existing composition devices
        for (int i = 0; i < devices2Count; i++)
        {
            if (devices2[i])
            {
                delete devices2[i];
                devices2[i] = nullptr;
            }
        }
        devices2Count = 0;

        // Load composition devices from JSON
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

            // Create composition devices based on type
            if (type == "led")
            {
                newDevice = new composition::Led(id);
            }
            else if (type == "button")
            {
                newDevice = new composition::Button(id);
            }
            else if (type == "test2")
            {
                newDevice = new composition::Test2(id);
            }
            else
            {
                MLOG_WARN("Unknown composition device type: %s", type.c_str());
                continue;
            }

            if (newDevice)
            {
                // Load config if device is serializable and config exists
                if (newDevice->hasMixin("serializable") && obj["config"].is<JsonObject>())
                {
                    // Use getSerializable() for RTTI-free access to ISerializable interface
                    ISerializable *serializable = newDevice->getSerializable();
                    if (serializable)
                    {
                        JsonDocument configDoc;
                        configDoc.set(obj["config"].as<JsonObject>());
                        serializable->jsonToConfig(configDoc);
                    }
                }

                // Add to device manager
                addDevice(newDevice);

                MLOG_INFO("Loaded composition device: %s (%s)", id.c_str(), type.c_str());
            }
        }

        MLOG_INFO("Loaded %d composition devices from %s", devices2Count, CONFIG_FILE);
    }
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
                doc.to<JsonObject>(); // Create empty object
            }
        }
        else
        {
            MLOG_ERROR("Failed to read existing configuration file, creating new one");
            doc.clear();
            doc.to<JsonObject>(); // Create empty object
        }
    }
    else
    {
        doc.to<JsonObject>(); // Create empty object
    }

    // Ensure we have a root object
    if (!doc.is<JsonObject>())
    {
        doc.clear();
        doc.to<JsonObject>();
    }

    JsonObject rootObj = doc.as<JsonObject>();

    // Replace devices array with current composition devices snapshot
    rootObj.remove("devices");
    JsonArray devicesArray = rootObj["devices"].to<JsonArray>();

    // Only save composition devices (DeviceBase)
    for (int i = 0; i < devices2Count; i++)
    {
        DeviceBase *device = devices2[i];
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

        // Only save config for devices that implement SerializableMixin (ISerializable)
        if (device->hasMixin("serializable"))
        {
            ISerializable *serializable = device->getSerializable();
            if (serializable)
            {
                JsonDocument configDoc;
                serializable->configToJson(configDoc);
                deviceObj["config"] = configDoc.as<JsonObject>();
            }
        }
    }

    // Save back to file
    File file = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (file)
    {
        serializeJson(doc, file);
        file.close();
        MLOG_INFO("Saved composition devices list to %s", CONFIG_FILE);

        // Log the saved JSON in pretty format
        // String prettyJson;
        // serializeJsonPretty(doc, prettyJson);
        // MLOG_INFO("Saved config JSON:\n%s", prettyJson.c_str());
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

            // Check if network settings exist
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
    // First, read the existing configuration
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
                doc.to<JsonObject>(); // Create empty object
            }
        }
        else
        {
            MLOG_ERROR("Failed to read existing configuration file, creating new one");
            doc.clear();
            doc.to<JsonObject>(); // Create empty object
        }
    }
    else
    {
        doc.to<JsonObject>(); // Create empty object
    }

    // Ensure we have a root object
    if (!doc.is<JsonObject>())
    {
        doc.clear();
        doc.to<JsonObject>();
    }

    JsonObject rootObj = doc.as<JsonObject>();

    // Add/update network settings
    JsonObject networkObj = rootObj["network"].to<JsonObject>();
    networkObj["ssid"] = settings.ssid;
    networkObj["password"] = settings.password;

    // Save back to file
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

DeviceManager::DeviceManager(NotifyClients callback) : devicesCount(0), taskDevicesCount(0), devices2Count(0), notifyClients(callback), hasClients(nullptr)
{
    // Initialize device array to nullptr
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        devices[i] = nullptr;
    }
    // Initialize task device array to nullptr
    for (int i = 0; i < MAX_TASK_DEVICES; i++)
    {
        taskDevices[i] = nullptr;
    }
    // Initialize composition device array to nullptr
    for (int i = 0; i < MAX_COMPOSITION_DEVICES; i++)
    {
        devices2[i] = nullptr;
    }
}

bool DeviceManager::addDevice(Device *device)
{
    if (devicesCount < MAX_DEVICES && device != nullptr)
    {
        devices[devicesCount] = device;
        devicesCount++;
        MLOG_INFO("Added device: %s (%s)", device->getId().c_str(), device->getName().c_str());
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

bool DeviceManager::addTaskDevice(TaskDevice *device)
{
    if (taskDevicesCount < MAX_TASK_DEVICES && device != nullptr)
    {
        taskDevices[taskDevicesCount] = device;
        taskDevicesCount++;
        MLOG_INFO("Added task device: %s", device->toString().c_str());
        return true;
    }

    if (device == nullptr)
    {
        MLOG_ERROR("Error: Cannot add null task device");
    }
    else
    {
        MLOG_ERROR("Error: Task device array is full, cannot add device: %s", device->getId().c_str());
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

void DeviceManager::getTaskDevices(TaskDevice **deviceList, int &count, int maxResults)
{
    count = 0;
    for (int i = 0; i < taskDevicesCount && count < maxResults; i++)
    {
        if (taskDevices[i] != nullptr)
        {
            deviceList[count] = taskDevices[i];
            count++;
        }
    }
}

void DeviceManager::setup()
{
    // Set the notify callback on all devices recursively
    std::function<void(Device *)> setCallbackRecursive = [&](Device *dev)
    {
        if (!dev)
            return;
        dev->setNotifyClients(notifyClients);
        dev->setHasClients(hasClients);
        for (Device *child : dev->getChildren())
        {
            setCallbackRecursive(child);
        }
    };

    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            setCallbackRecursive(devices[i]);
        }
    }

    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            devices[i]->setup();
        }
    }

    // Setup composition devices (DeviceBase)
    for (int i = 0; i < devices2Count; i++)
    {
        if (devices2[i])
        {
            devices2[i]->setup();
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

    // Call loop() on all composition devices
    for (int i = 0; i < devices2Count; i++)
    {
        if (devices2[i] != nullptr)
        {
            devices2[i]->loop();
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

TaskDevice *DeviceManager::getTaskDeviceById(const String &deviceId) const
{
    // Helper function for recursive search
    std::function<TaskDevice *(TaskDevice *)> findRecursive = [&](TaskDevice *dev) -> TaskDevice *
    {
        if (!dev)
            return nullptr;
        if (dev->getId() == deviceId)
            return dev;
        for (TaskDevice *child : dev->getChildren())
        {
            TaskDevice *found = findRecursive(child);
            if (found)
                return found;
        }
        return nullptr;
    };

    for (int i = 0; i < taskDevicesCount; i++)
    {
        if (taskDevices[i] != nullptr)
        {
            TaskDevice *found = findRecursive(taskDevices[i]);
            if (found)
                return found;
        }
    }
    return nullptr;
}

// Recursively search for the first device of the given type
Device *DeviceManager::getDeviceByType(const String &deviceType) const
{
    std::function<Device *(Device *)> findRecursive = [&](Device *dev) -> Device *
    {
        if (!dev)
            return nullptr;
        if (dev->getType() == deviceType)
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

ControllableTaskDevice *DeviceManager::getControllableTaskDeviceById(const String &deviceId) const
{
    TaskDevice *taskDev = getTaskDeviceById(deviceId);
    if (taskDev && taskDev->isControllable())
    {
        return static_cast<ControllableTaskDevice *>(taskDev);
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

std::vector<Device *> DeviceManager::getAllDevices()
{
    std::vector<Device *> allDevices;

    std::function<void(Device *)> collectRecursive = [&](Device *device)
    {
        if (!device)
            return;
        allDevices.push_back(device);
        for (Device *child : device->getChildren())
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

Device *DeviceManager::createDevice(const String &deviceType, const String &deviceId, JsonVariant config, NotifyClients notifyCallback, HasClients hasClientsCallback)
{
    Device *newDevice = nullptr;
    String lowerType = deviceType;
    lowerType.toLowerCase();

    if (lowerType == "led")
    {
        newDevice = new Led(deviceId, notifyCallback);
    }
    else if (lowerType == "buzzer")
    {
        newDevice = new Buzzer(deviceId, notifyCallback);
    }
    else if (lowerType == "button")
    {
        newDevice = new Button(deviceId, notifyCallback);
    }
    else if (lowerType == "stepper")
    {
        newDevice = new Stepper(deviceId, notifyCallback);
    }
    else if (lowerType == "pwmmotor")
    {
        newDevice = new PwmMotor(deviceId, notifyCallback);
    }
    else if (lowerType == "wheel")
    {
        newDevice = new Wheel(deviceId, notifyCallback);
    }
    else if (lowerType == "lift")
    {
        newDevice = new Lift(deviceId, notifyCallback);
    }
    else
    {
        MLOG_ERROR("Unknown device type: %s", deviceType.c_str());
        return nullptr;
    }

    if (newDevice != nullptr)
    {
        // Set hasClients callback if provided
        if (hasClientsCallback)
        {
            newDevice->setHasClients(hasClientsCallback);
        }
        
        // Apply configuration if provided
        if (config.is<JsonObject>())
        {
            JsonObject configObj = config.as<JsonObject>();
            newDevice->setConfig(&configObj);
        }

        // MLOG_INFO("Created device: %s (%s)", deviceId.c_str(), deviceType.c_str());
    }

    return newDevice;
}

bool DeviceManager::addDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    if (devicesCount >= MAX_DEVICES)
    {
        MLOG_ERROR("Cannot add device: Maximum device limit reached (%d)", MAX_DEVICES);
        return false;
    }

    // Check if device with same ID already exists
    if (getDeviceById(deviceId) != nullptr)
    {
        MLOG_ERROR("Cannot add device: Device with ID '%s' already exists", deviceId.c_str());
        return false;
    }

    Device *newDevice = createDevice(deviceType, deviceId, config, notifyClients, hasClients);
    if (newDevice == nullptr)
    {
        return false;
    }

    devices[devicesCount] = newDevice;
    devicesCount++;

    MLOG_INFO("Added device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
    return true;
}

// ======== Composition Device (DeviceBase) Methods ========

DeviceBase *DeviceManager::createCompositionDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    DeviceBase *newDevice = nullptr;
    String lowerType = deviceType;
    lowerType.toLowerCase();

    // Create composition devices based on type
    if (lowerType == "led")
    {
        newDevice = new composition::Led(deviceId);
    }
    else if (lowerType == "button")
    {
        newDevice = new composition::Button(deviceId);
    }
    else if (lowerType == "test2")
    {
        newDevice = new composition::Test2(deviceId);
    }
    else
    {
        MLOG_ERROR("Unknown composition device type: %s", deviceType.c_str());
        return nullptr;
    }

    // Load config if provided and device is serializable
    if (newDevice && newDevice->hasMixin("serializable") && config.is<JsonObject>())
    {
        ISerializable *serializable = newDevice->getSerializable();
        if (serializable)
        {
            JsonDocument configDoc;
            configDoc.set(config.as<JsonObject>());
            serializable->jsonToConfig(configDoc);
        }
    }

    return newDevice;
}

bool DeviceManager::addCompositionDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    if (devices2Count >= MAX_COMPOSITION_DEVICES)
    {
        MLOG_ERROR("Cannot add composition device: Maximum device limit reached (%d)", MAX_COMPOSITION_DEVICES);
        return false;
    }

    // Check if device with same ID already exists
    if (getCompositionDeviceById(deviceId) != nullptr)
    {
        MLOG_ERROR("Cannot add composition device: Device with ID '%s' already exists", deviceId.c_str());
        return false;
    }

    DeviceBase *newDevice = createCompositionDevice(deviceType, deviceId, config);
    if (newDevice == nullptr)
    {
        return false;
    }

    devices2[devices2Count] = newDevice;
    devices2Count++;

    MLOG_INFO("Added composition device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
    return true;
}

bool DeviceManager::addDevice(DeviceBase *device)
{
    if (devices2Count < MAX_COMPOSITION_DEVICES && device != nullptr)
    {
        devices2[devices2Count] = device;
        devices2Count++;
        MLOG_INFO("Added composition device: %s (%s)", device->getId().c_str(), device->getType().c_str());
        return true;
    }

    if (device == nullptr)
    {
        MLOG_ERROR("Error: Cannot add null composition device");
    }
    else
    {
        MLOG_ERROR("Error: Composition device array is full, cannot add device: %s", device->getId().c_str());
    }
    return false;
}

void DeviceManager::getCompositionDevices(DeviceBase **deviceList, int &count, int maxResults)
{
    count = 0;
    for (int i = 0; i < devices2Count && count < maxResults; i++)
    {
        if (devices2[i] != nullptr)
        {
            deviceList[count] = devices2[i];
            count++;
        }
    }
}

DeviceBase *DeviceManager::getCompositionDeviceById(const String &deviceId) const
{
    for (int i = 0; i < devices2Count; i++)
    {
        if (devices2[i] != nullptr)
        {
            DeviceBase *found = findCompositionDeviceRecursiveById(devices2[i], deviceId);
            if (found)
                return found;
        }
    }
    return nullptr;
}

DeviceBase *DeviceManager::findCompositionDeviceRecursiveById(DeviceBase *root, const String &deviceId) const
{
    if (!root)
        return nullptr;
    if (root->getId() == deviceId)
        return root;
    for (DeviceBase *child : root->getChildren())
    {
        DeviceBase *found = findCompositionDeviceRecursiveById(child, deviceId);
        if (found)
            return found;
    }
    return nullptr;
}
