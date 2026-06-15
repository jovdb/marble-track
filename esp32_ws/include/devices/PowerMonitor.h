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
        float minVoltage = 15.0f;            // 0% battery / low-voltage alert threshold (V)
        float maxVoltage = 21.0f;            // 100% battery reference voltage (V)
        unsigned long notifyIntervalMs = 10000; // How often to read and push state (ms)
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

    private:
        INA226_WE *_ina226 = nullptr;

        unsigned long _lastReadMs = 0;
        bool _voltageAlertActive = false;   // Tracks alert state to avoid repeated notifications

        /**
         * @brief Locate the I2C device, initialize the INA226 and update state.
         * Called from setup() and from control("init").
         */
        void init();

        /**
         * @brief Read voltage, current and power from INA226 and update _state.
         */
        void readMeasurements();
    };

} // namespace devices

#endif // COMPOSITION_POWERMONITOR_H
