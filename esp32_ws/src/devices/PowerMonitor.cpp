/**
 * @file PowerMonitor.cpp
 * @brief INA226-based voltage/current/power monitor device implementation
 */

#include "devices/PowerMonitor.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include "DeviceManager.h"
#include "devices/I2c.h"

extern DeviceManager deviceManager;

namespace devices
{

    PowerMonitor::PowerMonitor(const String &id)
        : Device(id, "powermonitor")
    {
    }

    PowerMonitor::~PowerMonitor()
    {
        if (_ina226)
        {
            delete _ina226;
            _ina226 = nullptr;
        }
    }

    void PowerMonitor::init()
    {
        // Clean up any previous INA226 instance
        if (_ina226)
        {
            delete _ina226;
            _ina226 = nullptr;
        }

        // Look up the I2C bus device
        devices::I2c *i2cDevice = nullptr;
        if (!_config.i2cDeviceId.isEmpty())
        {
            i2cDevice = deviceManager.getDeviceByIdAs<devices::I2c>(_config.i2cDeviceId);
        }

        if (!i2cDevice)
        {
            MLOG_ERROR("%s: Required I2C device '%s' not found", toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("NOT_FOUND", "I2C device '" + _config.i2cDeviceId + "' not found");
            _state.status = "Error";
            notifyStateChanged();
            return;
        }

        if (!i2cDevice->isSetup())
        {
            MLOG_ERROR("%s: I2C device '%s' is not set up", toString().c_str(), _config.i2cDeviceId.c_str());
            Device::setError("NOT_READY", "I2C device '" + _config.i2cDeviceId + "' is not set up");
            _state.status = "Error";
            notifyStateChanged();
            return;
        }

        // Create and initialise the INA226 driver
        _ina226 = new INA226_WE(&Wire, _config.i2cAddress);
        if (!_ina226->init())
        {
            MLOG_ERROR("%s: INA226 not found at address 0x%02X on I2C bus '%s'",
                       toString().c_str(), _config.i2cAddress, _config.i2cDeviceId.c_str());
            Device::setError("NO_ACK", "INA226 at 0x" + String(_config.i2cAddress, HEX) + " not responding — is it connected?");
            _state.status = "Error";
            notifyStateChanged();
            delete _ina226;
            _ina226 = nullptr;
            return;
        }

        // Configure: shunt resistance + expected max current for calibration register
        _ina226->setResistorRange(_config.shuntResistance, _config.maxCurrent);

        // Use 16-sample averaging to reduce noise on a marble-track power rail
        _ina226->setAverage(INA226_AVERAGE_16);

        // Continuous measurement mode (both shunt and bus)
        _ina226->setMeasureMode(INA226_CONTINUOUS);

        Device::clearError();
        _state.status = "Ready";
        _voltageAlertActive = false;
        notifyStateChanged();

        MLOG_INFO("%s: INA226 ready at 0x%02X on I2C bus '%s' (shunt=%.4f\u03a9, maxI=%.2fA)",
                  toString().c_str(),
                  _config.i2cAddress,
                  _config.i2cDeviceId.c_str(),
                  _config.shuntResistance,
                  _config.maxCurrent);
    }

    void PowerMonitor::setup()
    {
        Device::setup();
        setName(_config.name);
        init();
    }

    void PowerMonitor::teardown()
    {
        if (_ina226)
        {
            delete _ina226;
            _ina226 = nullptr;
        }
        Device::teardown();
    }

    void PowerMonitor::readMeasurements()
    {
        if (!_ina226)
            return;

        _state.voltage = _ina226->getBusVoltage_V();
        _state.current = _ina226->getCurrent_A();
        _state.watt = _ina226->getBusPower();
        _state.timestamp = millis();

        // Low-voltage alert: fire once on transition, clear once voltage recovers
        if (_state.voltage < _config.minVoltage && !_voltageAlertActive)
        {
            _voltageAlertActive = true;
            String msg = "Voltage " + String(_state.voltage, 2) + "V is below minimum " + String(_config.minVoltage, 2) + "V";
            broadcastNotification("LOW_VOLTAGE", msg, DeviceNotificationType::Warning);
            MLOG_WARN("%s: %s", toString().c_str(), msg.c_str());
        }
        else if (_state.voltage >= _config.minVoltage && _voltageAlertActive)
        {
            _voltageAlertActive = false;
            MLOG_INFO("%s: Voltage recovered to %.2fV", toString().c_str(), _state.voltage);
        }
    }

    void PowerMonitor::loop()
    {
        Device::loop();

        if (_state.status != "Ready" || !_ina226)
            return;

        unsigned long now = millis();
        if (now - _lastReadMs >= _config.notifyIntervalMs)
        {
            _lastReadMs = now;
            readMeasurements();
            notifyStateChanged();
        }
    }

    void PowerMonitor::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["status"] = _state.status;
        doc["voltage"] = _state.voltage;
        doc["current"] = _state.current;
        doc["watt"] = _state.watt;
        doc["timestamp"] = _state.timestamp;
    }

    bool PowerMonitor::control(const String &action, JsonObject *args)
    {
        if (action == "init")
        {
            init();
            return true;
        }
        return false;
    }

    void PowerMonitor::jsonToConfig(const JsonDocument &doc)
    {
        if (doc["name"].is<String>())
            _config.name = doc["name"].as<String>();
        if (doc["i2cDeviceId"].is<String>())
            _config.i2cDeviceId = doc["i2cDeviceId"].as<String>();
        if (doc["i2cAddress"].is<int>())
            _config.i2cAddress = doc["i2cAddress"].as<uint8_t>();
        if (doc["shuntResistance"].is<float>())
            _config.shuntResistance = doc["shuntResistance"].as<float>();
        if (doc["maxCurrent"].is<float>())
            _config.maxCurrent = doc["maxCurrent"].as<float>();
        if (doc["minVoltage"].is<float>())
            _config.minVoltage = doc["minVoltage"].as<float>();
        if (doc["maxVoltage"].is<float>())
            _config.maxVoltage = doc["maxVoltage"].as<float>();
        if (doc["notifyIntervalMs"].is<unsigned long>())
            _config.notifyIntervalMs = doc["notifyIntervalMs"].as<unsigned long>();
    }

    void PowerMonitor::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
        doc["i2cAddress"] = _config.i2cAddress;
        doc["shuntResistance"] = _config.shuntResistance;
        doc["maxCurrent"] = _config.maxCurrent;
        doc["minVoltage"] = _config.minVoltage;
        doc["maxVoltage"] = _config.maxVoltage;
        doc["notifyIntervalMs"] = _config.notifyIntervalMs;
    }

} // namespace devices
