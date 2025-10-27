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
#include "devices/Servo.h"
#include "devices/Stepper.h"
#include "devices/Wheel.h"
#include "devices/PwmMotor.h"
#include "devices/PwmDevice.h"
#include "devices/Lift.h"

static constexpr const char *CONFIG_FILE = "/config.json";

enum class DeviceType
{
    LED,
    BUZZER,
    BUTTON,
    SERVO,
    STEPPER,
    PWMMOTOR,
    PWM,
    WHEEL,
    LIFT,
    UNKNOWN
};

DeviceType stringToDeviceType(const String &type)
{
    if (type == "led")
        return DeviceType::LED;
    if (type == "buzzer")
        return DeviceType::BUZZER;
    if (type == "button")
        return DeviceType::BUTTON;
    if (type == "servo")
        return DeviceType::SERVO;
    if (type == "stepper")
        return DeviceType::STEPPER;
    if (type == "pwmmotor")
        return DeviceType::PWMMOTOR;
    if (type == "pwm")
        return DeviceType::PWM;
    if (type == "wheel")
        return DeviceType::WHEEL;
    if (type == "lift")
        return DeviceType::LIFT;
    return DeviceType::UNKNOWN;
}

void DeviceManager::loadDevicesFromJsonFile()
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        MLOG_INFO("File %s not found.", CONFIG_FILE);
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, FILE_READ);
    if (file)
    {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        // String prettyJson;
        // serializeJsonPretty(doc, prettyJson);
        // MLOG_INFO("Loaded JSON from config file:\n%s", prettyJson.c_str());

        if (!err && doc.is<JsonObject>())
        {
            JsonObject rootObj = doc.as<JsonObject>();

            // Check if devices array exists
            if (rootObj["devices"].is<JsonArray>())
            {
                JsonArray arr = rootObj["devices"];
                // Clear current devices
                for (int i = 0; i < devicesCount; i++)
                {
                    if (devices[i])
                    {
                        delete devices[i];
                        devices[i] = nullptr;
                    }
                }
                devicesCount = 0;

                std::map<String, Device *> loadedDevices;
                for (JsonObject obj : arr)
                {
                    String id = obj["id"] | "";
                    String type = obj["type"] | "";

                    if (loadedDevices.find(id) != loadedDevices.end())
                    {
                        MLOG_WARN("Duplicate device ID '%s' in config, skipping", id.c_str());
                        continue;
                    }

                    Device *newDevice = nullptr;
                    DeviceType devType = stringToDeviceType(type);
                    switch (devType)
                    {
                    case DeviceType::LED:
                    {
                        Led *led = new Led(id);
                        // Log obj[config] as json string
                        String configStr;
                        serializeJson(obj["config"], configStr);
                        MLOG_INFO("LED device config: %s", configStr.c_str());
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            led->setConfig(&config);
                        }
                        newDevice = led;
                        break;
                    }
                    case DeviceType::BUZZER:
                    {
                        Buzzer *buzzer = new Buzzer(id, "remove");
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            buzzer->setConfig(&config);
                        }
                        newDevice = buzzer;
                        break;
                    }
                    case DeviceType::BUTTON:
                    {
                        Button *button = new Button(id);
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            button->setConfig(&config);
                        }
                        newDevice = button;
                        break;
                    }
                    case DeviceType::SERVO:
                    {
                        ServoDevice *servo = new ServoDevice(id, "remove");
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            servo->setConfig(&config);
                        }
                        newDevice = servo;
                        break;
                    }
                    case DeviceType::STEPPER:
                    {
                        Stepper *stepper = new Stepper(id);
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            stepper->setConfig(&config);
                        }
                        newDevice = stepper;
                        break;
                    }
                    case DeviceType::PWMMOTOR:
                    {
                        PwmMotor *motor = new PwmMotor(id, "remove");
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            motor->setConfig(&config);
                        }
                        newDevice = motor;
                        break;
                    }
                    case DeviceType::PWM:
                    {
                        PwmDevice *pwm = new PwmDevice(id, "remove");
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            pwm->setConfig(&config);
                        }
                        newDevice = pwm;
                        break;
                    }
                    case DeviceType::WHEEL:
                    {
                        Wheel *wheel = new Wheel(id);
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            wheel->setConfig(&config);
                        }
                        newDevice = wheel;
                        break;
                    }
                    case DeviceType::LIFT:
                    {
                        Lift *lift = new Lift(id);
                        // Apply configuration from JSON config property if it exists
                        if (obj["config"].is<JsonObject>())
                        {
                            JsonObject config = obj["config"];
                            lift->setConfig(&config);
                        }
                        newDevice = lift;
                        break;
                    }
                    default:
                    {
                        MLOG_WARN("Unknown device type: %s", type.c_str());
                        continue;
                    }
                    }

                    if (newDevice)
                    {
                        loadedDevices[id] = newDevice;
                    }
                }

                // Collect all child IDs
                std::set<String> childIds;
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

                // Identify top-level devices and add to devices array
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

                // Link children
                for (JsonObject obj : arr)
                {
                    String id = obj["id"] | "";
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

                MLOG_INFO("Loaded %d devices from %s", loadedDevices.size(), CONFIG_FILE);
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

    // Replace devices array with current snapshot to avoid stale entries
    rootObj.remove("devices");
    JsonArray devicesArray = rootObj["devices"].to<JsonArray>();
    std::vector<Device *> allDevices = getAllDevices();
    for (Device *device : allDevices)
    {
        JsonObject deviceObj = devicesArray.add<JsonObject>();
        deviceObj["id"] = device->getId();
        deviceObj["type"] = device->getType();

        // Save children IDs
        JsonArray childrenArray = deviceObj["children"].to<JsonArray>();
        for (Device *child : device->getChildren())
        {
            childrenArray.add(child->getId());
        }

        // Save device configuration
        String configStr = device->getConfig();
        if (configStr.length() > 0)
        {
            DynamicJsonDocument configDoc(4096);
            DeserializationError err = deserializeJson(configDoc, configStr);
            if (!err && configDoc.is<JsonObject>())
            {
                deviceObj["config"] = configDoc.as<JsonObject>();
            }
            else
            {
                if (err)
                {
                    MLOG_WARN("Device %s: failed to deserialize config for persistence (%s)", device->getId().c_str(), err.c_str());
                }
                else
                {
                    MLOG_WARN("Device %s: config is not a JSON object, preserving raw string", device->getId().c_str());
                }
                deviceObj["config"] = serialized(configStr.c_str());
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
        MLOG_INFO("Configuration file not found, returning default network settings");
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
        // Set the callback on the newly added device if we have one
        if (notifyClients)
        {
            device->setNotifyClients(notifyClients);
        }
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

void DeviceManager::setup(NotifyClients notifyClients)
{
    this->notifyClients = notifyClients;

    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            devices[i]->setNotifyClients(notifyClients);
            devices[i]->setup();
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

Device *DeviceManager::createDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    Device *newDevice = nullptr;
    String lowerType = deviceType;
    lowerType.toLowerCase();

    DeviceType devType = stringToDeviceType(lowerType);
    switch (devType)
    {
    case DeviceType::LED:
        newDevice = new Led(deviceId);
        break;
    case DeviceType::BUZZER:
        newDevice = new Buzzer(deviceId, deviceId); // Using deviceId as name too
        break;
    case DeviceType::BUTTON:
        newDevice = new Button(deviceId);
        break;
    case DeviceType::SERVO:
        newDevice = new ServoDevice(deviceId, deviceId);
        break;
    case DeviceType::STEPPER:
        newDevice = new Stepper(deviceId);
        break;
    case DeviceType::PWMMOTOR:
        newDevice = new PwmMotor(deviceId, deviceId);
        break;
    case DeviceType::PWM:
        newDevice = new PwmDevice(deviceId, deviceId);
        break;
    case DeviceType::WHEEL:
        newDevice = new Wheel(deviceId);
        break;
    case DeviceType::LIFT:
        newDevice = new Lift(deviceId);
        break;
    default:
        MLOG_ERROR("Unknown device type: %s", deviceType.c_str());
        return nullptr;
    }

    if (newDevice != nullptr)
    {
        // Apply configuration if provided
        if (config.is<JsonObject>())
        {
            JsonObject configObj = config.as<JsonObject>();
            newDevice->setConfig(&configObj);
        }

        MLOG_INFO("Created device: %s (%s)", deviceId.c_str(), deviceType.c_str());
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

    Device *newDevice = createDevice(deviceType, deviceId, config);
    if (newDevice == nullptr)
    {
        return false;
    }

    devices[devicesCount] = newDevice;
    devicesCount++;

    // Set the callback on the newly added device if we have one
    if (notifyClients)
    {
        newDevice->setNotifyClients(notifyClients);
    }

    MLOG_INFO("Added device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
    return true;
}
