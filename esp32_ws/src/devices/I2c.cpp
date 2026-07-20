/**
 * @file I2c.cpp
 * @brief Implementation of I2C bus device
 */

#include "devices/I2c.h"
#include "Logging.h"
#include <vector>

namespace devices
{

    // Helper to translate Wire errors to strings
    static String getWireStatus(byte error)
    {
        switch (error)
        {
        case 0:
            return "OK";
        case 1:
            return "Data too long";
        case 2:
            return "NACK on Address";
        case 3:
            return "NACK on Data";
        case 4:
            return "Bus Error";
        case 5:
            return "Timeout";
        default:
            return "Error " + String(error);
        }
    }

    // Helper to parse hex string (e.g., "00 FF 12") to bytes
    static std::vector<uint8_t> parseHexString(const String &hex)
    {
        std::vector<uint8_t> bytes;
        String temp = hex;
        temp.replace(" ", "");
        temp.replace("0x", "");
        temp.replace(",", "");

        for (uint16_t i = 0; i < temp.length(); i += 2)
        {
            String part = temp.substring(i, i + 2);
            if (part.length() > 0)
            {
                bytes.push_back((uint8_t)strtol(part.c_str(), NULL, 16));
            }
        }
        return bytes;
    }

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

        if (!_state.lastOp.type.isEmpty())
        {
            JsonObject lastOp = doc["lastOp"].to<JsonObject>();
            lastOp["type"] = _state.lastOp.type;
            lastOp["address"] = _state.lastOp.address;
            lastOp["data"] = _state.lastOp.data;
            lastOp["length"] = _state.lastOp.length;
            lastOp["status"] = _state.lastOp.status;
            lastOp["timestamp"] = _state.lastOp.timestamp;
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

    bool I2c::control(const String &action, JsonObject *args)
    {
        if (action == "scan")
        {
            _state.foundAddresses = scanBus();
            notifyStateChanged();
            return true;
        }

        if (action == "write" && args)
        {
            int addr = (*args)["address"] | 0;
            String dataStr = (*args)["data"] | "";
            auto bytes = parseHexString(dataStr);

            Wire.beginTransmission(addr);
            for (auto b : bytes)
            {
                Wire.write(b);
            }
            byte err = Wire.endTransmission();

            _state.lastOp.type = "write";
            _state.lastOp.address = addr;
            _state.lastOp.data = dataStr;
            _state.lastOp.length = (int)bytes.size();
            _state.lastOp.status = getWireStatus(err);
            _state.lastOp.timestamp = millis();

            notifyStateChanged();
            return true;
        }

        if (action == "read" && args)
        {
            int addr = (*args)["address"] | 0;
            int len = (*args)["length"] | 1;

            int received = Wire.requestFrom(addr, len);
            String result = "";
            for (int i = 0; i < received; i++)
            {
                char buf[5];
                sprintf(buf, "%02X ", Wire.read());
                result += buf;
            }
            result.trim();

            _state.lastOp.type = "read";
            _state.lastOp.address = addr;
            _state.lastOp.data = result;
            _state.lastOp.length = len;
            _state.lastOp.status = (received == len ? "OK" : "Partial Read");
            _state.lastOp.timestamp = millis();

            notifyStateChanged();
            return true;
        }
        return false;
    }

} // namespace devices