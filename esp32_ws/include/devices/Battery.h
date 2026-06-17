/**
 * @file Battery.h
 * @brief Battery state device — derives voltage/percent from a linked PowerMonitor
 */

#ifndef COMPOSITION_BATTERY_H
#define COMPOSITION_BATTERY_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/StateMixin.h"

namespace devices
{

    struct BatteryConfig
    {
        String name = "Battery";
        String powerMonitorDeviceId = "";  // ID of the PowerMonitor device to read from
        float minVoltage = 15.0f;          // Voltage at 0 % (Li-ion 5S safe cutoff)
        float maxVoltage = 21.0f;          // Voltage at 100 % (Li-ion 5S full charge)
    };

    struct BatteryState
    {
        String status = "Error";     // "Ready" | "Error"
        float voltage = 0.0f;        // Last known bus voltage in V
        float batteryPercent = 0.0f; // 0–100 %
    };

    class Battery : public Device,
                    public ConfigMixin<Battery, BatteryConfig>,
                    public StateMixin<Battery, BatteryState>,
                    public ControllableMixin<Battery>,
                    public SerializableMixin<Battery>
    {
    public:
        explicit Battery(const String &id);
        ~Battery() override = default;

        void setup() override;

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &doc) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool _lowVoltageAlerted = false;
        bool _criticalVoltageAlerted = false;

        // Looks up the PowerMonitor, triggers a fresh read, updates _state, and
        // returns true when the PowerMonitor is healthy.
        bool updateFromPowerMonitor();
    };

} // namespace devices

#endif // COMPOSITION_BATTERY_H
