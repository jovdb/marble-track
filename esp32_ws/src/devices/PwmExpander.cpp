/**
 * @file PwmExpander.cpp
 * @brief PCA9685 16-channel 12-bit PWM expander device implementation
 */

#include "devices/PwmExpander.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/I2c.h"
#include <ArduinoJson.h>

extern DeviceManager deviceManager;

namespace devices
{
    PwmExpander::PwmExpander(const String &id)
        : Device(id, "pwmexpander")
    {
    }

    PwmExpander::~PwmExpander()
    {
        if (_driver)
        {
            delete _driver;
            _driver = nullptr;
        }
    }

    void PwmExpander::init()
    {
        const String prevState = _state.state;

        // Resolve I2C bus device
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
            MLOG_ERROR("%s: I2C device '%s' is not set up; configure it before this device",
                       toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("NOT_READY", "I2C device '" + _config.i2cDeviceId + "' is not set up");
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        // I2C health check
        Wire.beginTransmission(_config.i2cAddress);
        uint8_t error = Wire.endTransmission(true); // true = send stop bit, release bus

        if (error == 5)
        {
            String msg = "I\u00b2C timeout for PCA9685 at 0x" + String(_config.i2cAddress, HEX);
            Device::setError("TIMEOUT", msg);
            _state.state = "Error";
            notifyStateChanged();
            MLOG_WARN("%s: TIMEOUT: %s", toString().c_str(), msg.c_str());
            return;
        }
        else if (error != 0)
        {
            String errorCode;
            String errorMsg;
            switch (error)
            {
            case 2:
                errorCode = "NO_ACK";
                errorMsg = "PCA9685 at 0x" + String(_config.i2cAddress, HEX) + " not responding (no ACK \u2014 is it connected?)";
                break;
            case 4:
                errorCode = "BUS_ERROR";
                errorMsg = "I²C bus error for PCA9685 at 0x" + String(_config.i2cAddress, HEX) + " (check pull-ups)";
                break;
            case 5:
                errorCode = "TIMEOUT";
                errorMsg = "I²C timeout for PCA9685 at 0x" + String(_config.i2cAddress, HEX);
                break;
            default:
                errorCode = "I2C_ERROR_" + String(error);
                errorMsg = "I²C error " + String(error) + " for PCA9685 at 0x" + String(_config.i2cAddress, HEX);
                break;
            }
            Device::setError(errorCode, errorMsg);
            _state.state = "Error";
            notifyStateChanged();
            return;
        }

        // Create and initialise the Adafruit driver
        if (_driver)
        {
            delete _driver;
            _driver = nullptr;
        }
        _driver = new Adafruit_PWMServoDriver(_config.i2cAddress, Wire);
        _driver->begin();

        float freq = _config.frequency;
        if (freq < 24.0f || freq > 1526.0f)
        {
            MLOG_WARN("%s: Frequency %.1f Hz out of range [24-1526], using 50 Hz", toString().c_str(), freq);
            freq = 50.0f;
        }
        _driver->setPWMFreq(freq);

        Device::clearError();
        _state.state = "Ready";
        notifyStateChanged();
        MLOG_INFO("%s: PCA9685 ready at 0x%02X, %.1f Hz, 16 channels",
                  toString().c_str(), _config.i2cAddress, freq);

        if (prevState == "Error")
            deviceManager.reSetupDevicesUsingExpander(getId());
    }

    void PwmExpander::setup()
    {
        Device::setup();
        setName(_config.name);
        init();
    }

    void PwmExpander::teardown()
    {
        Device::teardown();

        // Do NOT communicate with the device here: the I2C bus (Wire) may already
        // be torn down, causing a NULL-buffer crash inside Wire::requestFrom().
        // Just release the driver object; PCA9685 retains its last state until
        // power-cycle, and setup() will re-initialise it cleanly.
        if (_driver)
        {
            delete _driver;
            _driver = nullptr;
        }
    }

    void PwmExpander::loop()
    {
        Device::loop();
        // No periodic work needed
    }

    std::vector<String> PwmExpander::getPins() const
    {
        // The PCA9685 uses the I2C bus; no direct GPIO pins are consumed
        return {};
    }

    bool PwmExpander::isDevicePresent() const
    {
        return _state.state == "Ready";
    }

    void PwmExpander::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["i2cDeviceId"].is<String>())
            _config.i2cDeviceId = config["i2cDeviceId"].as<String>();
        if (config["i2cAddress"].is<int>())
            _config.i2cAddress = static_cast<uint8_t>(config["i2cAddress"].as<int>());
        if (config["frequency"].is<float>() || config["frequency"].is<int>())
            _config.frequency = config["frequency"].as<float>();
    }

    void PwmExpander::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
        doc["i2cAddress"] = _config.i2cAddress;
        doc["frequency"] = _config.frequency;
    }

    void PwmExpander::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = _state.state;
    }

    bool PwmExpander::control(const String &action, JsonObject * /*args*/)
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

} // namespace devices
