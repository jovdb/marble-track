#include "pins/Pins.h"
#include <ArduinoJson.h>

// Factory function to create IPin from pin number (GPIO only)
pins::IPin* PinFactory::createPin(int pinNumber)
{
    PinConfig config;
    config.pin = pinNumber;
    config.pinType = PinType::GPIO;
    return createPin(config);
}

// Factory function to create IPin from PinConfig
pins::IPin* PinFactory::createPin(const PinConfig& config)
{
    switch (config.pinType)
    {
        case PinType::GPIO:
            return new pins::GpioPin();
        case PinType::I2C_PCF8574:
            return new pins::I2cExpanderPin(pins::I2cExpanderType::PCF8574, config.i2cAddress);
        case PinType::I2C_PCF8575:
            return new pins::I2cExpanderPin(pins::I2cExpanderType::PCF8575, config.i2cAddress);
        case PinType::I2C_MCP23017:
            return new pins::I2cExpanderPin(pins::I2cExpanderType::MCP23017, config.i2cAddress);
        default:
            return nullptr;
    }
}

// Parse pin config from JSON - handles both number and object formats
PinConfig PinFactory::jsonToConfig(const JsonDocument& doc)
{
    PinConfig config;

    // Check if it's a simple number (GPIO pin)
    if (doc.is<uint8_t>())
    {
        config.pinType = PinType::GPIO;
        config.pin = doc.as<uint8_t>();
        config.i2cAddress = 0x20; // Default, not used for GPIO
    }
    // Otherwise, treat it as an object with pin configuration
    else
    {
        // Parse pinType
        if (doc["pinType"].is<const char*>())
        {
            const char* pinTypeStr = doc["pinType"];
            if (strcmp(pinTypeStr, "GPIO") == 0)
                config.pinType = PinType::GPIO;
            else if (strcmp(pinTypeStr, "PCF8574") == 0)
                config.pinType = PinType::I2C_PCF8574;
            else if (strcmp(pinTypeStr, "PCF8575") == 0)
                config.pinType = PinType::I2C_PCF8575;
            else if (strcmp(pinTypeStr, "MCP23017") == 0)
                config.pinType = PinType::I2C_MCP23017;
        }

        // Parse pin
        if (doc["pin"].is<uint8_t>())
        {
            config.pin = doc["pin"];
        }

        // Parse i2cAddress
        if (doc["i2cAddress"].is<uint8_t>())
        {
            config.i2cAddress = doc["i2cAddress"];
        }
    }

    return config;
}

// Convert pin config to JSON
void PinFactory::configToJson(const PinConfig& config, JsonDocument& doc)
{
    // For GPIO, just store the pin number
    if (config.pinType == PinType::GPIO)
    {
        doc.set(config.pin);
        return;
    }

    // For I2C expanders, create an object
    JsonObject obj = doc.to<JsonObject>();

    // Set pinType
    switch (config.pinType)
    {
        case PinType::GPIO:
            obj["pinType"] = "GPIO";
            break;
        case PinType::I2C_PCF8574:
            obj["pinType"] = "PCF8574";
            break;
        case PinType::I2C_PCF8575:
            obj["pinType"] = "PCF8575";
            break;
        case PinType::I2C_MCP23017:
            obj["pinType"] = "MCP23017";
            break;
    }

    obj["pin"] = config.pin;
    obj["i2cAddress"] = config.i2cAddress;
}