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
        String powerMonitorDeviceId = ""; // ID of the PowerMonitor device to read from
        float minVoltage = 15.0f;         // Voltage at 0 % (Li-ion 5S safe cutoff)
        float maxVoltage = 20.3f;         // Voltage at 100 % (Li-ion 5S full charge)
        unsigned long refreshIntervalMs = 20000; // How often to refresh from PowerMonitor
    };

    struct BatteryState
    {
        String status = "Error";             // "Ready" | "Error"
        float voltage = 0.0f;                // Last known bus voltage in V
        float batteryPercent = 0.0f;         // 0–100 %
        unsigned long lastUpdatedMillis = 0; // Timestamp of last update
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
        void loop() override;

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &doc) override;
        void configToJson(JsonDocument &doc) override;

        // Looks up the PowerMonitor, triggers a fresh read, updates _state, and broadcasts
        // returns true when the PowerMonitor is healthy.
        bool refresh();

    private:
        bool _lowVoltageAlerted = false;
        bool _criticalVoltageAlerted = false;
    };

} // namespace devices

#endif // COMPOSITION_BATTERY_H
