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
        : Device(id, "ioexpander")
    {
    }

    IoExpander::~IoExpander()
    {
    }

    void IoExpander::init()
    {
        const String prevState = _state.state;

        // Get the I2C device
        devices::I2c *i2cDevice = nullptr;
        if (!_config.i2cDeviceId.isEmpty())
        {
            i2cDevice = deviceManager.getDeviceByIdAs<devices::I2c>(_config.i2cDeviceId);
        }

        if (!i2cDevice)
        {
            MLOG_ERROR("%s: Required I2C device '%s' not found", toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("NOT_FOUND", "I2C device '" + _config.i2cDeviceId + "' not found");
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        if (!i2cDevice->isSetup())
        {
            MLOG_ERROR("%s: I2C device '%s' is not set up, make sure it is configured before this device", toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("NOT_READY", "I2C device '" + _config.i2cDeviceId + "' is not set up");
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        auto i2cPins = i2cDevice->getPins();
        if (i2cPins.size() < 2)
        {
            MLOG_ERROR("%s: I2C device '%s' does not have SDA/SCL pins configured",
                       toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("CONFIG_ERROR", "I2C device '" + _config.i2cDeviceId + "' has no SDA/SCL pins");
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        int sdaPin = i2cPins[0].toInt();
        int sclPin = i2cPins[1].toInt();

        if (sdaPin < 0 || sclPin < 0)
        {
            MLOG_ERROR("%s: I2C device '%s' has invalid SDA/SCL pins (SDA=%d, SCL=%d)",
                       toString().c_str(), _config.i2cDeviceId.c_str(), sdaPin, sclPin);
            Device::setError("CONFIG_ERROR", "I2C device '" + _config.i2cDeviceId + "' has invalid pins");
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        Wire.beginTransmission(_config.i2cAddress);
        uint8_t error = Wire.endTransmission(true); // true = send stop bit, release bus

        if (error == 0)
        {
            Device::clearError();
            _state.state = "Ready";
            notifyStateChanged();
            MLOG_INFO("%s: Found %s at address 0x%02X on I2C bus '%s' (SDA=%d, SCL=%d) with %d pins",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      _config.i2cDeviceId.c_str(),
                      sdaPin,
                      sclPin,
                      getPinCount());

            if (prevState == "Error")
                deviceManager.reSetupDevicesUsingExpander(getId());
        }
        else if (error == 5)
        {
            String msg = "I\u00b2C timeout for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX);
            Device::setError("TIMEOUT", msg);
            _state.state = "Error";
            notifyStateChanged();
            MLOG_WARN("%s: TIMEOUT: %s", toString().c_str(), msg.c_str());
        }
        else
        {
            String errorCode;
            String errorMsg;
            switch (error)
            {
            case 2:
                errorCode = "NO_ACK";
                errorMsg = getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX) + " not responding (no ACK — is it connected?)";;
                break;
            case 4:
                errorCode = "BUS_ERROR";
                errorMsg = "I²C bus error for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX) + " (check pull-ups)";
                break;
            case 5:
                errorCode = "TIMEOUT";
                errorMsg = "I²C timeout for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX);
                break;
            default:
                errorCode = "I2C_ERROR_" + String(error);
                errorMsg = "I²C error " + String(error) + " for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX);
                break;
            }
            Device::setError(errorCode, errorMsg);
            _state.state = "Error";
            notifyStateChanged();
            MLOG_WARN("%s: %s not found at address 0x%02X on I2C bus '%s' (I2C error: %d)",
                      toString().c_str(),
                      getExpanderTypeString().c_str(),
                      _config.i2cAddress,
                      _config.i2cDeviceId.c_str(),
                      error);
        }
    }

    void IoExpander::setup()
    {
        Device::setup();
        setName(_config.name);
        init();
    }

    void IoExpander::teardown()
    {
        Device::teardown();
    }

    void IoExpander::loop()
    {
        Device::loop();

        // Periodically re-probe the I2C bus to detect recovery from transient errors
        // (e.g. the bus was locked up at startup but the device is now responding).
        constexpr unsigned long HEALTH_CHECK_INTERVAL_MS = 5000;
        unsigned long now = millis();
        if (now - _lastHealthCheckMs >= HEALTH_CHECK_INTERVAL_MS)
        {
            _lastHealthCheckMs = now;
            if (_state.state != "Ready")
            {
                init();
            }
        }
    }

    std::vector<String> IoExpander::getPins() const
    {
        // IO Expander doesn't use GPIO pins directly - it uses an I2C bus
        // The I2C device manages the SDA/SCL pins
        return std::vector<String>();
    }

    bool IoExpander::isDevicePresent() const
    {
        return _state.state == "Ready";
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

    void IoExpander::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = _state.state;
    }

    bool IoExpander::control(const String &action, JsonObject * /*args*/)
    {
        if (action == "init")
        {
            MLOG_INFO("%s: Re-initialising on request", toString().c_str());
            _state.state = "Init";
            notifyStateChanged();
            init();
            return true;
        }
        return false;
    }

    void IoExpander::reportRuntimeI2cError(uint8_t i2cError)
    {
        if (_state.state == "Ready")
        {
            String errorCode = (i2cError == 5) ? "TIMEOUT" : "I2C_ERROR";
            String msg = (i2cError == 5)
                             ? "I\u00b2C timeout for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX)
                             : "I\u00b2C error " + String(i2cError) + " for " + getExpanderTypeString() + " at 0x" + String(_config.i2cAddress, HEX);
            Device::setError(errorCode, msg);
            _state.state = "Error";
            notifyStateChanged();
        }
    }

} // namespace devices
