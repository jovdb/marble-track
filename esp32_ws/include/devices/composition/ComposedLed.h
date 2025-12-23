/**
 * @file ComposedLed.h
 * @brief Example LED device using composition pattern
 *
 * Demonstrates how to compose a device from mixins:
 * - DeviceBase for core identity/lifecycle
 * - RtosMixin for RTOS task support
 * - SaveableMixin for JSON config persistence
 * - ControllableMixin for WebSocket control
 * - StateChangeMixin for internal event subscriptions
 */

#ifndef COMPOSED_LED_H
#define COMPOSED_LED_H

#include <Arduino.h>
#include <atomic>

#include "devices/composition/DeviceBase.h"
#include "devices/composition/RtosMixin.h"
#include "devices/composition/SaveableMixin.h"
#include "devices/composition/ControllableMixin.h"
#include "devices/composition/StateChangeMixin.h"

/**
 * @class ComposedLed
 * @brief LED device using composition pattern
 *
 * Inherits from DeviceBase and multiple mixins to compose
 * a fully-featured LED controller with:
 * - RTOS task for non-blocking blink
 * - JSON config save/load
 * - WebSocket control
 * - State change notifications
 */
class ComposedLed : public DeviceBase,
                    public RtosMixin<ComposedLed>,
                    public SaveableMixin<ComposedLed>,
                    public ControllableMixin<ComposedLed>,
                    public StateChangeMixin<ComposedLed>
{
    // LED modes
    enum class Mode
    {
        OFF,
        ON,
        BLINKING
    };

public:
    /**
     * @brief Constructor
     * @param id Unique identifier for the LED
     */
    explicit ComposedLed(const String &id);

    // --- DeviceBase overrides ---
    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    // --- RtosMixin: task implementation ---
    void task();

    // --- SaveableMixin: config persistence ---
    void loadConfigFromJson(const JsonDocument &config) override;
    void saveConfigToJson(JsonDocument &doc) const override;

    // --- ControllableMixin: control and state ---
    bool handleControl(const String &action, JsonObject *args) override;
    void addStateToJson(JsonDocument &doc) const override;

    // --- LED-specific operations ---
    bool set(bool state);
    bool blink(unsigned long onTime = 500, unsigned long offTime = 500);
    void toggle();

private:
    int _pin = -1;
    Mode _mode = Mode::OFF;
    Mode _initialMode = Mode::OFF;

    // Blink timing
    unsigned long _blinkOnTime = 500;
    unsigned long _blinkOffTime = 500;

    // Physical state
    bool _isOn = false;

    // Thread-safe targets for RTOS task
    std::atomic<Mode> _targetMode{Mode::OFF};
    std::atomic<bool> _targetState{false};
    std::atomic<unsigned long> _targetBlinkOnTime{500};
    std::atomic<unsigned long> _targetBlinkOffTime{500};

    void applyInitialState();
    Mode modeFromString(const String &value) const;
    String modeToString(Mode mode) const;
};

#endif // COMPOSED_LED_H
