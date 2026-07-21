/**
 * @file Adxl345.cpp
 * @brief ADXL345 3-axis accelerometer device implementation
 */

#include "devices/Adxl345.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include "DeviceManager.h"
#include "devices/I2c.h"
#include <math.h>

extern DeviceManager deviceManager;

namespace devices
{

    Adxl345::Adxl345(const String &id)
        : Device(id, "adxl345")
    {
    }

    Adxl345::~Adxl345()
    {
        if (_adxl)
        {
            delete _adxl;
            _adxl = nullptr;
        }
    }

    void Adxl345::init()
    {
        if (_adxl)
        {
            delete _adxl;
            _adxl = nullptr;
        }

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

        _adxl = new Adafruit_ADXL345_Unified();
        
        // The Adafruit library uses the default Wire object by default.
        // We assume the I2c device has already called Wire.begin().
        if (!_adxl->begin(_config.i2cAddress))
        {
            MLOG_ERROR("%s: ADXL345 not found at address 0x%02X on I2C bus",
                       toString().c_str(), _config.i2cAddress);
            Device::setError("NO_ACK", "ADXL345 at 0x" + String(_config.i2cAddress, HEX) + " not responding");
            _state.status = "Error";
            notifyStateChanged();
            delete _adxl;
            _adxl = nullptr;
            return;
        }

        // Set range
        range_t range;
        switch (_config.range)
        {
        case 2: range = ADXL345_RANGE_2_G; break;
        case 4: range = ADXL345_RANGE_4_G; break;
        case 8: range = ADXL345_RANGE_8_G; break;
        default: range = ADXL345_RANGE_16_G; break;
        }
        _adxl->setRange(range);

        Device::clearError();
        _state.status = "Ready";
        notifyStateChanged();

        MLOG_INFO("%s: ADXL345 ready at 0x%02X (range=%dG)",
                  toString().c_str(), _config.i2cAddress, _config.range);
    }

    void Adxl345::setup()
    {
        Device::setup();
        setName(_config.name);
        init();
    }

    void Adxl345::teardown()
    {
        if (_adxl)
        {
            delete _adxl;
            _adxl = nullptr;
        }
        Device::teardown();
    }

    void Adxl345::readSensor()
    {
        if (!_adxl)
            return;

        sensors_event_t event;
        _adxl->getEvent(&event);

        // Apply offsets
        float rawX = event.acceleration.x;
        float rawY = event.acceleration.y;
        float rawZ = event.acceleration.z;

        _state.x = rawX - _config.offsetX;
        _state.y = rawY - _config.offsetY;
        _state.z = rawZ - _config.offsetZ;

        // Calculate roll and pitch in degrees
        // roll = atan2(y, z)
        // pitch = atan2(-x, sqrt(y*y + z*z))
        _state.roll = atan2(_state.y, _state.z) * 180.0 / M_PI;
        _state.pitch = atan2(-_state.x, sqrt(_state.y * _state.y + _state.z * _state.z)) * 180.0 / M_PI;

        _state.lastUpdatedMillis = millis();
    }

    void Adxl345::loop()
    {
        Device::loop();

        if (_adxl && _state.status == "Ready")
        {
            unsigned long now = millis();
            if (now - _state.lastUpdatedMillis > _config.refreshIntervalMs)
            {
                readSensor();
            }
        }
    }

    void Adxl345::addDeviceStateToJson(JsonDocument &doc)
    {
        // Ensure we have semi-fresh data when state is requested
        readSensor();
        
        doc["status"] = _state.status;
        doc["x"] = _state.x;
        doc["y"] = _state.y;
        doc["z"] = _state.z;
        doc["roll"] = _state.roll;
        doc["pitch"] = _state.pitch;
        doc["lastUpdated"] = _state.lastUpdatedMillis;
    }

    bool Adxl345::control(const String &action, JsonObject *args)
    {
        if (action == "refresh")
        {
            readSensor();
            notifyStateChanged();
            return true;
        }
        
        if (action == "calibrate")
        {
            if (!_adxl) return false;
            
            sensors_event_t event;
            _adxl->getEvent(&event);
            
            // Assume board is level: X=0, Y=0, Z=9.81
            _config.offsetX = event.acceleration.x;
            _config.offsetY = event.acceleration.y;
            _config.offsetZ = event.acceleration.z - 9.80665f;
            
            deviceManager.saveDevicesToJsonFile();
            readSensor();
            notifyStateChanged();
            MLOG_INFO("%s: Calibrated. New offsets: X=%.2f, Y=%.2f, Z=%.2f", 
                toString().c_str(), _config.offsetX, _config.offsetY, _config.offsetZ);
            return true;
        }
        return false;
    }

    void Adxl345::jsonToConfig(const JsonDocument &doc)
    {
        _config.name = doc["name"] | "Accelerometer";
        _config.i2cDeviceId = doc["i2cDeviceId"] | "";
        _config.i2cAddress = doc["i2cAddress"] | 0x53;
        _config.range = doc["range"] | 16;
        _config.refreshIntervalMs = doc["refreshIntervalMs"] | 100;
        _config.offsetX = doc["offsetX"] | 0.0f;
        _config.offsetY = doc["offsetY"] | 0.0f;
        _config.offsetZ = doc["offsetZ"] | 0.0f;
        
        if (isSetup()) {
            init();
        }
    }

    void Adxl345::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["i2cDeviceId"] = _config.i2cDeviceId;
        doc["i2cAddress"] = _config.i2cAddress;
        doc["range"] = _config.range;
        doc["refreshIntervalMs"] = _config.refreshIntervalMs;
        doc["offsetX"] = _config.offsetX;
        doc["offsetY"] = _config.offsetY;
        doc["offsetZ"] = _config.offsetZ;
    }

} // namespace devices
