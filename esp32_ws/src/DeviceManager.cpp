#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <functional>
#include "LittleFS.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Led.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Servo.h"
#include "devices/Stepper.h"
#include "devices/Wheel.h"
#include "devices/Lift.h"
#include "devices/MarbleController.h"
#include "devices/IoExpander.h"
#include "devices/I2c.h"
#include "devices/mixins/SerializableMixin.h"

static constexpr const char *CONFIG_FILE = "/config.json";

Device *DeviceManager::createDevice(const String &deviceId, const String &deviceType)
{
    String upperType = deviceType;
    upperType.toUpperCase();

    // MLOG_DEBUG("Creating device of type: '%s' with ID: '%s'", upperType.c_str(), deviceId.c_str());

    if (upperType == "LED")
    {
        return new devices::Led(deviceId);
    }
    else if (upperType == "BUTTON")
    {
        return new devices::Button(deviceId);
    }
    else if (upperType == "BUZZER")
    {
        return new devices::Buzzer(deviceId);
    }
    else if (upperType == "SERVO")
    {
        return new devices::Servo(deviceId);
    }
    else if (upperType == "STEPPER")
    {
        return new devices::Stepper(deviceId);
    }
    else if (upperType == "WHEEL")
    {
        return new devices::Wheel(deviceId);
    }
    else if (upperType == "LIFT")
    {
        return new devices::Lift(deviceId);
    }
    else if (upperType == "MARBLECONTROLLER")
    {
        return new devices::MarbleController(deviceId);
    }
    else if (upperType == "IOEXPANDER")
    {
        return new devices::IoExpander(deviceId);
    }
    else if (upperType == "I2C")
    {
        return new devices::I2c(deviceId);
    }

    MLOG_WARN("Unknown device type: %s", upperType.c_str());
    return nullptr;
}

/**
 * @brief Recursively apply config to devices from JSON
 * @param device The device to apply config to
 * @param deviceObj JSON object with optional children array and config
 */
void DeviceManager::loadDeviceConfigFromJson(Device *device, JsonObject deviceObj)
{
    if (!device)
    {
        MLOG_WARN("Null device passed to loadDeviceConfigFromJson");
        return;
    }

    // Apply config if device is serializable and config exists
    if (device->hasMixin("serializable"))
    {
        ISerializable *serializable = mixins::SerializableRegistry::get(device->getId());
        if (serializable)
        {
            MLOG_DEBUG("%s: loading JSON config", device->toString().c_str());
            JsonDocument configDoc;
            if (deviceObj["config"].is<JsonObject>())
            {
                configDoc.set(deviceObj["config"].as<JsonObject>());
            }
            serializable->jsonToConfig(configDoc);
            device->setName(configDoc["name"] | device->getId());
        }
    }

    // Recursively apply config to children by walking device children
    for (Device *childDevice : device->getChildren())
    {
        const String childId = childDevice->getId();
        JsonObject childObj;
        bool found = false;
        if (deviceObj["children"].is<JsonArray>())
        {
            JsonArray childrenArray = deviceObj["children"].as<JsonArray>();
            for (JsonObject obj : childrenArray)
            {
                if (childId == (obj["id"] | ""))
                {
                    childObj = obj;
                    found = true;
                    break;
                }
            }
        }
        if (found)
        {
            loadDeviceConfigFromJson(childDevice, childObj);
        }
        else
        {
            MLOG_WARN("No JSON config found for child device: %s", childId.c_str());
        }
    }
}

void DeviceManager::loadDevicesFromJsonFile()
{
    MLOG_DEBUG("Loading devices from JSON file: %s", CONFIG_FILE);

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

    // Clear existing devices
    deleteAllDevices();

    JsonObject rootObj = doc.as<JsonObject>();
    if (!rootObj["devices"].is<JsonArray>())
    {
        MLOG_INFO("No devices array found in config file");
        return;
    }

    // Load only root devices from the array (children are created by parent devices during config application)
    JsonArray arr = rootObj["devices"];
    if (arr.size() == 0)
    {
        MLOG_INFO("No devices to load");
        return;
    }

    int roots = 0;
    for (JsonObject deviceObj : arr)
    {
        const String id = deviceObj["id"] | "";
        const String type = deviceObj["type"] | "";
        Device *newDevice = createDevice(id, type);
        if (newDevice)
        {
            // Disable this if esp32 keeps rebooting due to config error
            loadDeviceConfigFromJson(newDevice, deviceObj);
            addDevice(newDevice);
            roots++;
        }
    }

    // Log result
    MLOG_INFO("Loaded %d root devices from %s", roots, CONFIG_FILE);
    MLOG_DEBUG("-----------------------");
}

/**
 * @brief Saves all devices (including children) to the config file
 *
 * Saves devices in a flat list by:
 * 1. Walking the device tree to get all devices (parents + children)
 * 2. Serializing each device
 * 3. For devices with the serializable mixin, including their config property
 *
 * This ensures child devices (e.g., LED and Button in composite devices) are
 * persisted alongside their parent devices.
 */
void DeviceManager::saveDevicesToJsonFile()
{
    // First, read the existing configuration to preserve other properties
    // like network settings
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

    // Replace only devices array with current devices snapshot
    rootObj.remove("devices");

    // Create a new array of the current configured devices
    JsonArray devicesArray = rootObj["devices"].to<JsonArray>();
    addDevicesToJsonArray(devicesArray);

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

/**
 * @brief Recursively add a device and its children to JSON using tree structure
 * @param device Device to serialize
 * @param deviceObj JSON object to populate
 */
void DeviceManager::addDeviceToJsonObject(Device *device, JsonObject deviceObj)
{
    if (!device)
        return;

    deviceObj["id"] = device->getId();
    deviceObj["type"] = device->getType();

    // Recursively add children as nested objects in children array
    JsonArray childrenArray = deviceObj["children"].to<JsonArray>();
    for (Device *child : device->getChildren())
    {
        JsonObject childObj = childrenArray.add<JsonObject>();
        addDeviceToJsonObject(child, childObj);
    }

    // Only save config for devices that implement SerializableMixin
    if (device->hasMixin("serializable"))
    {
        ISerializable *serializable = mixins::SerializableRegistry::get(device->getId());
        if (serializable)
        {
            JsonDocument configDoc;
            serializable->configToJson(configDoc);
            deviceObj["config"] = configDoc.as<JsonObject>();
        }
    }
}

void DeviceManager::addDevicesToJsonArray(JsonArray &devicesArray)
{
    // Add only root devices (those in the devices array) with their full tree
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            JsonObject deviceObj = devicesArray.add<JsonObject>();
            addDeviceToJsonObject(devices[i], deviceObj);
        }
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

bool DeviceManager::addDevice(Device *device)
{
    if (devicesCount < MAX_DEVICES && device != nullptr)
    {
        devices[devicesCount] = device;
        devicesCount++;
        MLOG_DEBUG("Added device: %s", device->toString().c_str());
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

    Device *newDevice = createDevice(deviceId, deviceType);

    if (!newDevice)
    {
        return false;
    }

    MLOG_DEBUG("Device added: %s", newDevice->toString().c_str());

    // Load config if device is serializable and config exists
    if (newDevice->hasMixin("serializable") && config.is<JsonObject>())
    {
        ISerializable *serializable = mixins::SerializableRegistry::get(deviceId);
        if (serializable)
        {
            MLOG_DEBUG("Loading config for device %s", newDevice->toString().c_str());
            JsonDocument configDoc;
            configDoc.set(config.as<JsonObject>());
            serializable->jsonToConfig(configDoc);
        }
    }

    devices[devicesCount] = newDevice;
    devicesCount++;

    MLOG_INFO("Added device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
    return true;
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

void DeviceManager::setup()
{
    MLOG_DEBUG("DeviceManager setup started (root only devices)");
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            devices[i]->setup();
        }
    }
    MLOG_DEBUG("DeviceManager setup ended");
    MLOG_DEBUG("-----------------------");
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

Device *DeviceManager::getDeviceById(const String &deviceId) const
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            Device *found = findDeviceRecursiveById(devices[i], deviceId);
            if (found)
                return found;
        }
    }
    return nullptr;
}

Device *DeviceManager::findDeviceRecursiveById(Device *root, const String &deviceId) const
{
    if (!root)
        return nullptr;
    if (root->getId() == deviceId)
        return root;
    for (Device *child : root->getChildren())
    {
        Device *found = findDeviceRecursiveById(child, deviceId);
        if (found)
            return found;
    }
    return nullptr;
}

Device *DeviceManager::getDeviceByType(const String &deviceType) const
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i] != nullptr)
        {
            Device *found = findDeviceRecursiveByType(devices[i], deviceType);
            if (found)
                return found;
        }
    }
    return nullptr;
}

Device *DeviceManager::findDeviceRecursiveByType(Device *root, const String &deviceType) const
{
    if (!root)
        return nullptr;
    if (root->getType() == deviceType)
        return root;
    for (Device *child : root->getChildren())
    {
        Device *found = findDeviceRecursiveByType(child, deviceType);
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

bool DeviceManager::reorderDevices(const std::vector<String> &deviceIds)
{
    // Validate that all device IDs exist and count matches
    if ((int)deviceIds.size() != devicesCount)
    {
        MLOG_WARN("Reorder failed: device count mismatch (expected %d, got %d)", devicesCount, (int)deviceIds.size());
        return false;
    }

    // Create a temporary array to hold the reordered devices
    Device *reordered[MAX_DEVICES];
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        reordered[i] = nullptr;
    }

    // Map each device ID to its position in the new order
    for (int i = 0; i < (int)deviceIds.size(); i++)
    {
        const String &id = deviceIds[i];
        bool found = false;

        for (int j = 0; j < devicesCount; j++)
        {
            if (devices[j] != nullptr && devices[j]->getId() == id)
            {
                reordered[i] = devices[j];
                found = true;
                break;
            }
        }

        if (!found)
        {
            MLOG_WARN("Reorder failed: device '%s' not found", id.c_str());
            return false;
        }
    }

    // Check for duplicates (a device appearing twice in the new order)
    for (int i = 0; i < (int)deviceIds.size(); i++)
    {
        for (int j = i + 1; j < (int)deviceIds.size(); j++)
        {
            if (reordered[i] == reordered[j])
            {
                MLOG_WARN("Reorder failed: duplicate device ID");
                return false;
            }
        }
    }

    // Copy the reordered array back to devices
    for (int i = 0; i < devicesCount; i++)
    {
        devices[i] = reordered[i];
    }

    MLOG_INFO("Devices reordered successfully");
    return true;
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

void DeviceManager::deleteAllDevices()
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
