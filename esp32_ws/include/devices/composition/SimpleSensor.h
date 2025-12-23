/**
 * @file SimpleSensor.h
 * @brief Example sensor device using composition WITHOUT RTOS
 *
 * Demonstrates a simpler device that doesn't need an RTOS task.
 * Uses loop() for polling instead.
 */

#ifndef SIMPLE_SENSOR_H
#define SIMPLE_SENSOR_H

#include <Arduino.h>

#include "devices/composition/DeviceBase.h"
#include "devices/composition/SaveableMixin.h"
#include "devices/composition/ControllableMixin.h"

/**
 * @class SimpleSensor
 * @brief Simple analog sensor using composition without RTOS
 *
 * Demonstrates that you can skip RtosMixin when not needed.
 * Reads analog value in loop() and notifies on significant changes.
 */
class SimpleSensor : public DeviceBase,
                     public SaveableMixin<SimpleSensor>,
                     public ControllableMixin<SimpleSensor>
{
public:
    explicit SimpleSensor(const String &id);

    // --- DeviceBase overrides ---
    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    // --- SaveableMixin: config persistence ---
    void loadConfigFromJson(const JsonDocument &config) override;
    void saveConfigToJson(JsonDocument &doc) const override;

    // --- ControllableMixin: control and state ---
    bool handleControl(const String &action, JsonObject *args) override;
    void addStateToJson(JsonDocument &doc) const override;

    // --- Sensor-specific ---
    int read();
    int getValue() const { return _value; }

private:
    int _pin = -1;
    int _value = 0;
    int _lastNotifiedValue = 0;
    int _threshold = 10;            // Minimum change to trigger notification
    unsigned long _pollInterval = 100; // ms between reads
    unsigned long _lastPollTime = 0;
};

#endif // SIMPLE_SENSOR_H
