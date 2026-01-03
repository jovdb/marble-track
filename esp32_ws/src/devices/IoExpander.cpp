/**
 * @file IoExpander.cpp
 * @brief I2C IO Expander device implementation
 */

#include "devices/IoExpander.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{
    IoExpander::IoExpander(const String &id)
        : Device(id, "ioexpander"), _isPresent(false)
    {
    }

    IoExpander::~IoExpander()
    {
    }

    void IoExpander::setup()
    {
        Device::setup();

        // Set the device name
        setName(_config.name);

        // Initialize I2C with configured pins
        Wire.begin(_config.sdaPin, _config.sclPin);

        // Check if device is present
        Wire.beginTransmission(_config.i2cAddress);
        uint8_t error = Wire.endTransmission();
        _isPresent = (error == 0);

        if (_isPresent)
        {
            MLOG_INFO("%s: Found %s at address 0x%02X (SDA=%d, SCL=%d) with %d pins",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      _config.sdaPin,
                      _config.sclPin,
                      getPinCount());
        }
        else
        {
            MLOG_WARN("%s: %s not found at address 0x%02X (I2C error: %d)",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      error);
        }
    }

    void IoExpander::loop()
    {
        Device::loop();
        // No periodic work needed - pins are managed individually
    }

    std::vector<String> IoExpander::getPins() const
    {
        // Return SDA and SCL pins as used GPIO pins
        std::vector<String> pins;
        pins.push_back(String(_config.sdaPin));
        pins.push_back(String(_config.sclPin));
        return pins;
    }

    bool IoExpander::isDevicePresent() const
    {
        return _isPresent;
    }

    int IoExpander::getPinCount() const
    {
        switch (_config.expanderType)
        {
        case IoExpanderType::PCF8574:
            return 8;
        case IoExpanderType::PCF8575:
        case IoExpanderType::MCP23017:
            return 16;
        default:
            return 0;
        }
    }

    String IoExpander::getExpanderTypeString() const
    {
        switch (_config.expanderType)
        {
        case IoExpanderType::PCF8574:
            return "PCF8574";
        case IoExpanderType::PCF8575:
            return "PCF8575";
        case IoExpanderType::MCP23017:
            return "MCP23017";
        default:
            return "Unknown";
        }
    }

    IoExpanderType IoExpander::stringToExpanderType(const String &typeStr) const
    {
        if (typeStr.equalsIgnoreCase("PCF8574"))
            return IoExpanderType::PCF8574;
        if (typeStr.equalsIgnoreCase("PCF8575"))
            return IoExpanderType::PCF8575;
        if (typeStr.equalsIgnoreCase("MCP23017"))
            return IoExpanderType::MCP23017;
        return IoExpanderType::PCF8574; // Default
    }

    void IoExpander::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
        if (config["expanderType"].is<String>())
        {
            _config.expanderType = stringToExpanderType(config["expanderType"].as<String>());
        }
        if (config["i2cAddress"].is<int>())
        {
            _config.i2cAddress = config["i2cAddress"].as<int>();
        }
        if (config["sdaPin"].is<int>())
        {
            _config.sdaPin = config["sdaPin"].as<int>();
        }
        if (config["sclPin"].is<int>())
        {
            _config.sclPin = config["sclPin"].as<int>();
        }
    }

    void IoExpander::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["expanderType"] = getExpanderTypeString();
        doc["i2cAddress"] = _config.i2cAddress;
        doc["sdaPin"] = _config.sdaPin;
        doc["sclPin"] = _config.sclPin;
    }

} // namespace devices
