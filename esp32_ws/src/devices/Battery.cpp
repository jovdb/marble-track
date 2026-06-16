/**
 * @file Battery.cpp
 * @brief Battery state device — derives voltage/percent from a linked PowerMonitor
 */

#include "devices/Battery.h"
#include "devices/PowerMonitor.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include "DeviceManager.h"

extern DeviceManager deviceManager;

namespace devices
{

    Battery::Battery(const String &id)
        : Device(id, "battery")
    {
    }

    void Battery::setup()
    {
        Device::setup();
        setName(_config.name);
        // Attempt an initial read so state isn't empty after boot
        updateFromPowerMonitor();
    }

    bool Battery::updateFromPowerMonitor()
    {
        PowerMonitor *pm = deviceManager.getDeviceByIdAs<PowerMonitor>(_config.powerMonitorDeviceId);

        if (!pm)
        {
            if (!_config.powerMonitorDeviceId.isEmpty())
            {
                MLOG_ERROR("%s: PowerMonitor '%s' not found", toString().c_str(), _config.powerMonitorDeviceId.c_str());
                Device::setError("NOT_FOUND", "PowerMonitor '" + _config.powerMonitorDeviceId + "' not found");
            }
            else
            {
                Device::setError("NOT_CONFIGURED", "No PowerMonitor device selected");
            }
            _state.status = "Error";
            return false;
        }

        // Trigger a fresh INA226 read
        pm->readMeasurements();

        if (!pm->isReady())
        {
            Device::setError("PM_ERROR", "PowerMonitor '" + _config.powerMonitorDeviceId + "' is in error state");
            _state.status = "Error";
            return false;
        }

        float voltage = pm->getVoltage();
        float pct = 0.0f;
        if (_config.maxVoltage > _config.minVoltage)
        {
            pct = (voltage - _config.minVoltage) / (_config.maxVoltage - _config.minVoltage) * 100.0f;
            pct = max(0.0f, min(100.0f, pct));
        }

        _state.voltage = voltage;
        _state.batteryPercent = pct;
        _state.status = "Ready";
        Device::clearError();

        // Low-voltage alert — fires once on transition, clears on recovery
        if (voltage < _config.minVoltage && !_lowVoltageAlertActive)
        {
            _lowVoltageAlertActive = true;
            String msg = "Battery voltage " + String(voltage, 2) + "V is below " + String(_config.minVoltage, 2) + "V (0%)";
            broadcastNotification("LOW_VOLTAGE", msg, DeviceNotificationType::Warning);
            MLOG_WARN("%s: %s", toString().c_str(), msg.c_str());
        }
        else if (voltage >= _config.minVoltage && _lowVoltageAlertActive)
        {
            _lowVoltageAlertActive = false;
            MLOG_INFO("%s: Battery voltage recovered to %.2fV", toString().c_str(), voltage);
        }

        return true;
    }

    void Battery::addDeviceStateToJson(JsonDocument &doc)
    {
        updateFromPowerMonitor();
        doc["status"] = _state.status;
        doc["voltage"] = _state.voltage;
        doc["batteryPercent"] = _state.batteryPercent;
    }

    bool Battery::control(const String &action, JsonObject * /*args*/)
    {
        if (action == "refresh")
        {
            updateFromPowerMonitor();
            notifyStateChanged();
            return true;
        }
        return false;
    }

    void Battery::jsonToConfig(const JsonDocument &doc)
    {
        if (doc["name"].is<String>())
            _config.name = doc["name"].as<String>();
        if (doc["powerMonitorDeviceId"].is<String>())
            _config.powerMonitorDeviceId = doc["powerMonitorDeviceId"].as<String>();
        if (doc["minVoltage"].is<float>())
            _config.minVoltage = doc["minVoltage"].as<float>();
        if (doc["maxVoltage"].is<float>())
            _config.maxVoltage = doc["maxVoltage"].as<float>();
    }

    void Battery::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["powerMonitorDeviceId"] = _config.powerMonitorDeviceId;
        doc["minVoltage"] = _config.minVoltage;
        doc["maxVoltage"] = _config.maxVoltage;
    }

} // namespace devices
