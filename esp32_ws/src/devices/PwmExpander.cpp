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

    void PwmExpander::setup()
    {
        Device::setup();
        setName(_config.name);

        // Resolve I2C bus device
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

        if (!i2cDevice->isSetup())
        {
            MLOG_ERROR("%s: I2C device '%s' is not set up; configure it before this device",
                       toString().c_str(), _config.i2cDeviceId.c_str());
            _isPresent = false;
            return;
        }

        // Quick I2C presence check
        Wire.beginTransmission(_config.i2cAddress);
        uint8_t error = Wire.endTransmission();
        _isPresent = (error == 0);

        if (!_isPresent)
        {
            MLOG_WARN("%s: PCA9685 not found at address 0x%02X on I2C bus '%s' (I2C error: %d)",
                      toString().c_str(), _config.i2cAddress, _config.i2cDeviceId.c_str(), error);
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

        MLOG_INFO("%s: PCA9685 ready at 0x%02X, %.1f Hz, 16 channels",
                  toString().c_str(), _config.i2cAddress, freq);
    }

    void PwmExpander::teardown()
    {
        Device::teardown();

        if (_driver)
        {
            // Turn off all channels
            for (int ch = 0; ch < 16; ch++)
                _driver->setPin(ch, 0, false);

            delete _driver;
            _driver = nullptr;
        }
        _isPresent = false;
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
        return _isPresent;
    }

    void PwmExpander::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["i2cDeviceId"].is<String>())
            _config.i2cDeviceId = config["i2cDeviceId"].as<String>();
        if (config["i2cAddress"].is<int>())
            _config.i2cAddress = static_cast<uint8_t>(config["i2cAddress"].as<int>());
        if (config["frequency"].is<float>())
            _config.frequency = config["frequency"].as<float>();
    }

    void PwmExpander::configToJson(JsonDocument &doc)
    {
        doc["name"]        = _config.name;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
        doc["i2cAddress"]  = _config.i2cAddress;
        doc["frequency"]   = _config.frequency;
    }

} // namespace devices
