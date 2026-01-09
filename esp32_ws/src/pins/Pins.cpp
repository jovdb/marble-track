#include "pins/Pins.h"
#include <ArduinoJson.h>
#include <Logging.h>

// Factory function to create IPin from pin number (GPIO only)
pins::IPin *PinFactory::createPin(int pinNumber)
{
    PinConfig config;
    config.pin = pinNumber;
    config.pinType = PinType::GPIO;
    return createPin(config);
}

// Factory function to create IPin from PinConfig
pins::IPin *PinFactory::createPin(const PinConfig &config)
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
PinConfig PinFactory::jsonToConfig(const JsonDocument &doc)
{
    PinConfig config;

    // Early exit if doc is null or missing
    if (doc.isNull())
    {
        config.pinType = PinType::GPIO;
        config.pin = -1;
        config.i2cAddress = 0;
        return config;
    }

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
        if (doc["pinType"].is<const char *>())
        {
            const char *pinTypeStr = doc["pinType"];
            if (strcmp(pinTypeStr, "PCF8574") == 0)
                config.pinType = PinType::I2C_PCF8574;
            else if (strcmp(pinTypeStr, "PCF8575") == 0)
                config.pinType = PinType::I2C_PCF8575;
            else if (strcmp(pinTypeStr, "MCP23017") == 0)
                config.pinType = PinType::I2C_MCP23017;
            else // default to GPIO
                config.pinType = PinType::GPIO;
        }

        if (doc["pin"].is<uint8_t>())
        {
            config.pin = doc["pin"];
        }
        else
        {
            config.pin = -1; // Invalid pin
        }

        // Parse i2cAddress
        if (doc["i2cAddress"].is<uint8_t>())
        {
            config.i2cAddress = doc["i2cAddress"];
        }
        else
        {
            config.i2cAddress = 0;
        }
    }

    return config;
}

// Convert pin config to JSON
void PinFactory::configToJson(const PinConfig &config, JsonDocument &doc)
{

    // Set pinType
    switch (config.pinType)
    {
    case PinType::GPIO:
        doc["pinType"] = "GPIO";
        doc["pin"] = config.pin;
        break;
    case PinType::I2C_PCF8574:
        doc["pinType"] = "PCF8574";
        doc["pin"] = config.pin;
        doc["i2cAddress"] = config.i2cAddress;
        break;
    case PinType::I2C_PCF8575:
        doc["pinType"] = "PCF8575";
        doc["pin"] = config.pin;
        doc["i2cAddress"] = config.i2cAddress;
        break;
    case PinType::I2C_MCP23017:
        doc["pinType"] = "MCP23017";
        doc["pin"] = config.pin;
        doc["i2cAddress"] = config.i2cAddress;
        break;
    }
}