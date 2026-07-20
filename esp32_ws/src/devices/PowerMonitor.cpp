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

        float voltage = _ina226->getBusVoltage_V();
        float current = _ina226->getCurrent_A();
        uint8_t i2cErr = _ina226->getI2cErrorCode();

        // INA226 bus voltage is capped at 36 V by hardware.
        // Values above that indicate a wrong I2C address (e.g. address conflict
        // with a PCA9685 PWM expander, which also defaults to 0x40).
        if (i2cErr != 0 || voltage > 40.0f)
        {
            String msg;
            if (i2cErr != 0)
                msg = "I\u00b2C error " + String(i2cErr) + " reading INA226 at 0x" + String(_config.i2cAddress, HEX);
            else
                msg = "Implausible voltage " + String(voltage, 2) + "V — check I\u00b2C address (conflict with another 0x" + String(_config.i2cAddress, HEX) + " device?)";

            MLOG_ERROR("%s: %s", toString().c_str(), msg.c_str());
            Device::setError("BAD_READ", msg);
            _state.status = "Error";
            notifyStateChanged();
            return;
        }

        _state.voltage = voltage;
        _state.current = current;
        _state.watt = voltage * current;
        _state.timestamp = millis();
    }

    void PowerMonitor::loop()
    {
        Device::loop();

        // Health check: re-initialise every 5 s while in error state
        unsigned long now = millis();
        if (now - _lastHealthCheckMs >= 5000)
        {
            _lastHealthCheckMs = now;
            if (_state.status != "Ready" || !_ina226)
                init();
        }
    }

    void PowerMonitor::addDeviceStateToJson(JsonDocument &doc)
    {
        if (_state.status == "Error" && _ina226)
            init();
        // Always do a fresh read so the website gets up-to-date values on every request
        if (_state.status == "Ready" && _ina226)
            readMeasurements();

        doc["status"] = _state.status;
        doc["voltage"] = _state.voltage;
        doc["current"] = _state.current;
        doc["watt"] = _state.watt;
        doc["timestamp"] = _state.timestamp;

        JsonArray addresses = doc["foundAddresses"].to<JsonArray>();
        for (int addr : _state.foundAddresses)
        {
            addresses.add(addr);
        }
    }

    bool PowerMonitor::control(const String &action, JsonObject *args)
    {
        if (action == "init")
        {
            init();
            return true;
        }
        else if (action == "scan")
        {
            devices::I2c *i2cDevice = nullptr;
            if (!_config.i2cDeviceId.isEmpty())
            {
                i2cDevice = deviceManager.getDeviceByIdAs<devices::I2c>(_config.i2cDeviceId);
            }

            if (i2cDevice)
            {
                _state.foundAddresses = i2cDevice->scanBus();
                notifyStateChanged();
                return true;
            }
            return false;
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
    }

    void PowerMonitor::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
        doc["i2cAddress"] = _config.i2cAddress;
        doc["shuntResistance"] = _config.shuntResistance;
        doc["maxCurrent"] = _config.maxCurrent;
    }

} // namespace devices
