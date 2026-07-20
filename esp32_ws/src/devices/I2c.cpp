/**
 * @file I2c.cpp
 * @brief Implementation of I2C bus device
 */

#include "devices/I2c.h"
#include "Logging.h"

namespace devices
{

    I2c::I2c(const String &id) : Device(id, "i2c")
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
        setName(getConfig().name);

        Wire.end(); // Ensure any previous instance is closed

        const auto &config = getConfig();
        if (config.sdaPin >= 0 && config.sclPin >= 0)
        {
            Wire.begin(config.sdaPin, config.sclPin);
            clearError();
            _state.state = "Ready";
            notifyStateChanged();
            MLOG_INFO("%s: I2C bus initialized on SDA=%d, SCL=%d", toString().c_str(), config.sdaPin, config.sclPin);
        }
        else
        {
            setError("CONFIG_ERROR", "Invalid pins SDA=" + String(config.sdaPin) + ", SCL=" + String(config.sclPin));
            _state.state = "Error";
            notifyStateChanged();
            MLOG_WARN("%s: I2C bus not initialized: invalid pins SDA=%d, SCL=%d", toString().c_str(), config.sdaPin, config.sclPin);
        }
    }

    void I2c::teardown()
    {
        Device::teardown();

        Wire.end();

        const auto &config = getConfig();
        if (config.sdaPin >= 0)
        {
            pinMode(config.sdaPin, INPUT);
        }
        if (config.sclPin >= 0)
        {
            pinMode(config.sclPin, INPUT);
        }
        clearError();
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

    void I2c::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = _state.state;
        JsonArray addresses = doc["foundAddresses"].to<JsonArray>();
        for (int addr : _state.foundAddresses)
        {
            addresses.add(addr);
        }
    }

    std::vector<int> I2c::scanBus()
    {
        std::vector<int> found;
        const auto &config = getConfig();
        if (config.sdaPin < 0 || config.sclPin < 0)
        {
            return found;
        }

        MLOG_INFO("%s: Starting I2C scan...", toString().c_str());

        for (byte address = 1; address < 127; address++)
        {
            Wire.beginTransmission(address);
            byte error = Wire.endTransmission();

            if (error == 0)
            {
                found.push_back(address);
                MLOG_INFO("%s: Found device at 0x%02X", toString().c_str(), address);
            }
        }

        MLOG_INFO("%s: Scan complete, found %d devices", toString().c_str(), found.size());
        return found;
    }

    bool I2c::control(const String &action, JsonObject * /*args*/)
    {
        if (action == "scan")
        {
            _state.foundAddresses = scanBus();
            notifyStateChanged();
            return true;
        }
        return false;
    }

} // namespace devices