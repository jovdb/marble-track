/**
 * @file PowerMonitor.h
 * @brief INA226-based voltage/current/power monitor device
 */

#ifndef COMPOSITION_POWERMONITOR_H
#define COMPOSITION_POWERMONITOR_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/StateMixin.h"
#include <Wire.h>
#include <INA226_WE.h>

namespace devices
{

    struct PowerMonitorConfig
    {
        String name = "Power Monitor";
        String i2cDeviceId = "";             // ID of the I2C bus device to use
        uint8_t i2cAddress = 0x40;           // INA226 I2C address (0x40–0x4F)
        float shuntResistance = 0.1f;        // Shunt resistor value in ohms
        float maxCurrent = 3.2f;             // Expected max current in amps (for calibration)
    };

    struct PowerMonitorState
    {
        String status = "Error";  // "Ready" | "Error"
        float voltage = 0.0f;     // Bus voltage in V
        float current = 0.0f;     // Current in A
        float watt = 0.0f;        // Power in W
        unsigned long timestamp = 0; // millis() at last reading
    };

    class PowerMonitor : public Device,
                         public ConfigMixin<PowerMonitor, PowerMonitorConfig>,
                         public StateMixin<PowerMonitor, PowerMonitorState>,
                         public ControllableMixin<PowerMonitor>,
                         public SerializableMixin<PowerMonitor>
    {
    public:
        explicit PowerMonitor(const String &id);
        ~PowerMonitor() override;

        void setup() override;
        void teardown() override;
        void loop() override;

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &doc) override;
        void configToJson(JsonDocument &doc) override;

        // Read fresh values from INA226 into _state. Called on every state request
        // and by Battery devices that reference this monitor.
        void readMeasurements();

        float getVoltage() const { return _state.voltage; }
        bool isReady() const { return _state.status == "Ready" && _ina226 != nullptr; }

    private:
        INA226_WE *_ina226 = nullptr;
        unsigned long _lastHealthCheckMs = 0;

        void init();
    };

} // namespace devices

#endif // COMPOSITION_POWERMONITOR_H
