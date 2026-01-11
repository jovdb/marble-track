/**
 * @file I2c.cpp
 * @brief Implementation of I2C bus device
 */

#include "devices/I2c.h"
#include "Logging.h"

namespace devices
{

    I2c::I2c(const String &id) : Device(id, "I2C")
    {
        // Set default config
        setConfig(I2cConfig());
    }

    I2c::~I2c()
    {
        // Cleanup if needed
    }

    void I2c::setup()
    {
        Device::setup();

        const auto &config = getConfig();
        if (config.sdaPin >= 0 && config.sclPin >= 0)
        {
            Wire.begin(config.sdaPin, config.sclPin);
            MLOG_INFO("I2C bus initialized on SDA=%d, SCL=%d", config.sdaPin, config.sclPin);
        }
        else
        {
            MLOG_WARN("I2C bus not initialized: invalid pins SDA=%d, SCL=%d", config.sdaPin, config.sclPin);
        }
    }

    std::vector<String> I2c::getPins() const
    {
        const auto &config = getConfig();
        return {String(config.sdaPin), String(config.sclPin)};
    }

    void I2c::jsonToConfig(const JsonDocument &config)
    {
        I2cConfig newConfig = getConfig();

        if (config["name"].is<String>())
        {
            newConfig.name = config["name"].as<String>();
        }
        if (config["sdaPin"].is<int>())
        {
            newConfig.sdaPin = config["sdaPin"].as<int>();
        }
        if (config["sclPin"].is<int>())
        {
            newConfig.sclPin = config["sclPin"].as<int>();
        }

        setConfig(newConfig);
        setName(newConfig.name);
    }

    void I2c::configToJson(JsonDocument &doc)
    {
        const auto &config = getConfig();
        doc["name"] = config.name;
        doc["sdaPin"] = config.sdaPin;
        doc["sclPin"] = config.sclPin;
    }

} // namespace devices