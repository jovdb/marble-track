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

static constexpr const char *DEVICES_LIST_FILE = "/devices.json";

void DeviceManager::loadDevicesFromJsonFile()
{
    if (!LittleFS.exists(DEVICES_LIST_FILE))
    {
        MLOG_INFO("Devices JSON file not found.");
        return;
    }

    File file = LittleFS.open(DEVICES_LIST_FILE, FILE_READ);
    if (file)
    {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();
        if (!err && doc.is<JsonArray>())
        {
            JsonArray arr = doc.as<JsonArray>();
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
                /*
                std::vector<int> pins;
                if (obj["pins"].is<JsonArray>())
                {
                    for (int pin : obj["pins"].as<JsonArray>())
                    {
                        pins.push_back(pin);
                    }
                }
                    */
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
            MLOG_INFO("Loaded devices from %s", DEVICES_LIST_FILE);
        }
        else
        {
            MLOG_ERROR("Failed to parse devices JSON file");
        }
    }
    else
    {
        MLOG_ERROR("Failed to open devices JSON file for reading");
    }
}

void DeviceManager::saveDevicesToJsonFile()
{
    File file = LittleFS.open(DEVICES_LIST_FILE, FILE_WRITE);
    if (file)
    {
        JsonDocument doc;
        JsonArray devicesArray = doc.to<JsonArray>();
        for (int i = 0; i < devicesCount; i++)
        {
            if (devices[i])
            {
                JsonObject deviceObj = devicesArray.add<JsonObject>();
                deviceObj["id"] = devices[i]->getId();
                deviceObj["name"] = devices[i]->getName();
                deviceObj["type"] = devices[i]->getType();
                std::vector<int> pins = devices[i]->getPins();
                JsonArray pinsArray = deviceObj["pins"].to<JsonArray>();
                for (int pin : pins)
                {
                    pinsArray.add(pin);
                }
            }
        }
        serializeJson(devicesArray, file);
        file.close();
        MLOG_INFO("Saved devices list to %s", DEVICES_LIST_FILE);
    }
    else
    {
        MLOG_ERROR("Failed to open %s for writing", DEVICES_LIST_FILE);
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
