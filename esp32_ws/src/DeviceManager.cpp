#include <ArduinoJson.h>
#include <vector>
#include "LittleFS.h"
#include "esp_log.h"
#include "DeviceManager.h"
#include "devices/Led.h"
#include "devices/Buzzer.h"
#include "devices/Button.h"
#include "devices/Servo.h"
#include "devices/Stepper.h"
#include "devices/Wheel.h"

static const char *TAG = "DeviceManager";
static constexpr const char *DEVICES_LIST_FILE = "/devices.json";

void DeviceManager::loadDevicesFromJsonFile()
{
    if (!LittleFS.exists(DEVICES_LIST_FILE))
    {
        ESP_LOGI(TAG, "Devices JSON file not found.");
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
                        // If pins are provided in JSON, use the first pin
                        // if (obj["pins"].is<JsonArray>() && obj["pins"].size() > 0) {
                        //     int pin = obj["pins"][0];
                        //     led->setup(pin);
                        // }
                        devices[devicesCount++] = led;
                    }
                    else if (type == "buzzer")
                    {
                        Buzzer *buzzer = new Buzzer(id, name);
                        devices[devicesCount++] = buzzer;
                    }
                    else if (type == "button")
                    {
                        Button *button = new Button(id, name);
                        devices[devicesCount++] = button;
                    }
                    else if (type == "servo")
                    {
                        ServoDevice *servo = new ServoDevice(id, name);
                        devices[devicesCount++] = servo;
                    }
                    // else if (type == "stepper")
                    // {
                    //     Stepper *stepper = new Stepper(id, name);
                    //     devices[devicesCount++] = stepper;
                    // }

                    else
                    {
                        ESP_LOGW(TAG, "Unknown device type: %s", type.c_str());
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "Maximum device limit reached, cannot load more devices");
                    break;
                }
            }
            ESP_LOGI(TAG, "Loaded devices from %s", DEVICES_LIST_FILE);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse devices JSON file");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open devices JSON file for reading");
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
        ESP_LOGI(TAG, "Saved devices list to %s", DEVICES_LIST_FILE);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open %s for writing", DEVICES_LIST_FILE);
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
        ESP_LOGI(TAG, "Added device: %s (%s)", device->getId().c_str(), device->getName().c_str());
        return true;
    }

    if (device == nullptr)
    {
        ESP_LOGE(TAG, "Error: Cannot add null device");
    }
    else
    {
        ESP_LOGE(TAG, "Error: Device array is full, cannot add device: %s", device->getId().c_str());
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
