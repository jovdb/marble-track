#include <ArduinoJson.h>
#include <vector>
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
#include "devices/Pwm.h"
#include "devices/PwdDevice.h"

static constexpr const char *CONFIG_FILE = "/config.json";

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

        String prettyJson;
        serializeJsonPretty(doc, prettyJson);
        MLOG_INFO("Loaded JSON from config file:\n%s", prettyJson.c_str());

        if (!err && doc.is<JsonObject>())
        {
            JsonObject rootObj = doc.as<JsonObject>();

            // Check if devices array exists
            if (rootObj["devices"].is<JsonArray>())
            {
                JsonArray arr = rootObj["devices"];
                // loadDevicesFromJson implementation (re-add here if needed)
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
                for (JsonObject obj : arr)
                {
                    String id = obj["id"] | "";
                    String name = obj["name"] | "";
                    String type = obj["type"] | "";

                    if (devicesCount < MAX_DEVICES)
                    {
                        if (type == "led")
                        {
                            Led *led = new Led(id);
                            led->setName(name);

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

                            devices[devicesCount++] = led;
                        }
                        else if (type == "buzzer")
                        {
                            Buzzer *buzzer = new Buzzer(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                buzzer->setConfig(&config);
                            }

                            devices[devicesCount++] = buzzer;
                        }
                        else if (type == "button")
                        {
                            Button *button = new Button(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                button->setConfig(&config);
                            }

                            devices[devicesCount++] = button;
                        }
                        else if (type == "servo")
                        {
                            ServoDevice *servo = new ServoDevice(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                servo->setConfig(&config);
                            }

                            devices[devicesCount++] = servo;
                        }
                        else if (type == "stepper")
                        {
                            Stepper *stepper = new Stepper(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                stepper->setConfig(&config);
                            }

                            devices[devicesCount++] = stepper;
                        }
                        else if (type == "pwmmotor")
                        {
                            PwmMotor *motor = new PwmMotor(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                motor->setConfig(&config);
                            }

                            devices[devicesCount++] = motor;
                        }
                        else if (type == "pwm")
                        {
                            Pwm *pwm = new Pwm(id, name);

                            // Apply configuration from JSON config property if it exists
                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                pwm->setConfig(&config);
                            }

                            devices[devicesCount++] = pwm;
                        }
                        else if (type == "pwddevice")
                        {
                            PwdDevice *device = new PwdDevice(id, name);

                            if (obj["config"].is<JsonObject>())
                            {
                                JsonObject config = obj["config"];
                                device->setConfig(&config);
                            }

                            devices[devicesCount++] = device;
                        }
                        else
                        {
                            MLOG_WARN("Unknown device type: %s", type.c_str());
                        }
                    }
                    else
                    {
                        MLOG_WARN("Maximum device limit reached, cannot load more devices");
                        break;
                    }
                }
                MLOG_INFO("Loaded devices from %s", CONFIG_FILE);
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
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            JsonObject deviceObj = devicesArray.add<JsonObject>();
            deviceObj["id"] = devices[i]->getId();
            deviceObj["name"] = devices[i]->getName();
            deviceObj["type"] = devices[i]->getType();

            // Save device configuration
            String configStr = devices[i]->getConfig();
            if (configStr.length() > 0)
            {
                JsonDocument configDoc;
                DeserializationError err = deserializeJson(configDoc, configStr);
                if (!err && configDoc.is<JsonObject>())
                {
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

        // Log the saved JSON in pretty format
        String prettyJson;
        serializeJsonPretty(doc, prettyJson);
        MLOG_INFO("Saved config JSON:\n%s", prettyJson.c_str());
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

void DeviceManager::setup(StateChangeCallback callback)
{
    for (int i = 0; i < devicesCount; i++)
    {
        if (devices[i])
        {
            devices[i]->setStateChangeCallback(callback);
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

Device *DeviceManager::createDevice(const String &deviceType, const String &deviceId, JsonVariant config)
{
    Device *newDevice = nullptr;
    String lowerType = deviceType;
    lowerType.toLowerCase();

    if (lowerType == "led")
    {
        newDevice = new Led(deviceId);
    }
    else if (lowerType == "buzzer")
    {
        newDevice = new Buzzer(deviceId, deviceId); // Using deviceId as name too
    }
    else if (lowerType == "button")
    {
        newDevice = new Button(deviceId, deviceId);
    }
    else if (lowerType == "servo")
    {
        newDevice = new ServoDevice(deviceId, deviceId);
    }
    else if (lowerType == "stepper")
    {
        newDevice = new Stepper(deviceId, deviceId);
    }
    else if (lowerType == "pwmmotor")
    {
        newDevice = new PwmMotor(deviceId, deviceId);
    }
    else if (lowerType == "pwm")
    {
        newDevice = new Pwm(deviceId, deviceId);
    }
    else if (lowerType == "pwddevice")
    {
        newDevice = new PwdDevice(deviceId, deviceId);
    }
    else
    {
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

    MLOG_INFO("Added device to array: %s (%s)", deviceId.c_str(), deviceType.c_str());
    return true;
}
