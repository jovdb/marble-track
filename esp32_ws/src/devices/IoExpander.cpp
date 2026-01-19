/**
 * @file IoExpander.cpp
 * @brief I2C IO Expander device implementation
 */

#include "devices/IoExpander.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include "DeviceManager.h"
#include "devices/I2c.h"

// External reference to device manager
extern DeviceManager deviceManager;

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

        // Get the I2C device
        devices::I2c *i2cDevice = nullptr;
        if (!_config.i2cDeviceId.isEmpty())
        {
            i2cDevice = deviceManager.getDeviceByIdAs<devices::I2c>(_config.i2cDeviceId);
        }

        if (!i2cDevice)
        {
            MLOG_ERROR("%s: Required I2C device '%s' not found", toString().c_str(), _config.i2cDeviceId.c_str());
            _isPresent = false;
            return;
        }

        // Get SDA and SCL pins from the I2C device
        auto i2cPins = i2cDevice->getPins();
        if (i2cPins.size() < 2)
        {
            MLOG_ERROR("%s: I2C device '%s' does not have SDA/SCL pins configured", 
                      toString().c_str(), _config.i2cDeviceId.c_str());
            _isPresent = false;
            return;
        }

        int sdaPin = i2cPins[0].toInt();
        int sclPin = i2cPins[1].toInt();

        // End any previous I2C setup
        Wire.end();

        // Initialize I2C with pins from the I2C device
        Wire.begin(sdaPin, sclPin);

        // Check if device is present
        Wire.beginTransmission(_config.i2cAddress);
        uint8_t error = Wire.endTransmission();
        _isPresent = (error == 0);

        if (_isPresent)
        {
            MLOG_INFO("%s: Found %s at address 0x%02X on I2C bus '%s' (SDA=%d, SCL=%d) with %d pins",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      _config.i2cDeviceId.c_str(),
                      sdaPin,
                      sclPin,
                      getPinCount());
        }
        else
        {
            MLOG_WARN("%s: %s not found at address 0x%02X on I2C bus '%s' (I2C error: %d)",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      _config.i2cDeviceId.c_str(),
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
        // IO Expander doesn't use GPIO pins directly - it uses an I2C bus
        // The I2C device manages the SDA/SCL pins
        return std::vector<String>();
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
        if (config["i2cDeviceId"].is<String>())
        {
            _config.i2cDeviceId = config["i2cDeviceId"].as<String>();
        }
    }

    void IoExpander::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["expanderType"] = getExpanderTypeString();
        doc["i2cAddress"] = _config.i2cAddress;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
    }

} // namespace devices
