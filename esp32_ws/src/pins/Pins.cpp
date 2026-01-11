#include "pins/Pins.h"
#include <ArduinoJson.h>
#include <Logging.h>
#include <map>
#include "DeviceManager.h"
#include "devices/IoExpander.h"

// Extern device manager for pin creation
extern DeviceManager deviceManager;

// Static cache for resolved expander addresses
static std::map<String, uint8_t> expanderAddresses;

// Setup method to resolve expander IDs to I2C addresses
void PinFactory::setup()
{
    expanderAddresses.clear();
    
    // Get all IoExpander devices and cache their addresses
    auto devices = deviceManager.getAllDevices();
    for (auto device : devices)
    {
        if (device->getType() == "ioexpander")
        {
            auto expander = static_cast<devices::IoExpander*>(device);
            expanderAddresses[device->getId()] = expander->getI2cAddress();
        }
    }
}

// Factory function to create IPin from pin number (GPIO only)
pins::IPin *PinFactory::createPin(int pinNumber)
{
    PinConfig config;
    config.pin = pinNumber;
    config.expanderId = ""; // Empty string for GPIO
    return createPin(config);
}

// Factory function to create IPin from PinConfig
pins::IPin *PinFactory::createPin(const PinConfig &config)
{
    // If no expanderId, it's a GPIO pin
    if (config.expanderId.isEmpty())
    {
        return new pins::GpioPin();
    }
    
    // Look up expander device by ID
    auto expander = deviceManager.getDeviceByIdAs<devices::IoExpander>(config.expanderId);
    if (!expander)
    {
        MLOG_ERROR("PinFactory: Expander device '%s' not found", config.expanderId.c_str());
        return nullptr;
    }
    
    // Get expander type and address
    auto expanderType = expander->getExpanderType();
    uint8_t i2cAddress = expander->getI2cAddress();
    
    // Convert IoExpanderType to I2cExpanderType
    pins::I2cExpanderType pinExpanderType;
    switch (expanderType)
    {
    case devices::IoExpanderType::PCF8574:
        pinExpanderType = pins::I2cExpanderType::PCF8574;
        break;
    case devices::IoExpanderType::PCF8575:
        pinExpanderType = pins::I2cExpanderType::PCF8575;
        break;
    case devices::IoExpanderType::MCP23017:
        pinExpanderType = pins::I2cExpanderType::MCP23017;
        break;
    default:
        MLOG_ERROR("PinFactory: Unknown expander type for device '%s'", config.expanderId.c_str());
        return nullptr;
    }
    
    return new pins::I2cExpanderPin(pinExpanderType, i2cAddress);
}

// Parse pin config from JSON - requires object format
PinConfig PinFactory::jsonToConfig(const JsonDocument &doc)
{
    PinConfig config;

    // Early exit if doc is null or missing
    if (doc.isNull())
    {
        config.pin = -1;
        config.expanderId = "";
        return config;
    }

    // Only support object format
    if (doc["pin"].is<int>())
    {
        config.pin = doc["pin"];
    }
    else
    {
        config.pin = -1; // Invalid pin
    }

    // Parse expanderId
    if (doc["expanderId"].is<const char *>() || doc["expanderId"].is<String>())
    {
        config.expanderId = doc["expanderId"].as<String>();
    }
    else
    {
        config.expanderId = "";
    }

    return config;
}

// Convert pin config to JSON - always outputs object format
void PinFactory::configToJson(const PinConfig &config, JsonDocument &doc)
{
    // Always output as object format
    doc["pin"] = config.pin;
    doc["expanderId"] = config.expanderId;
}