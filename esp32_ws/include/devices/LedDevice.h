/**
 * @file LedDevice.h
 * @brief LED device implementation using TaskDevice
 */

#ifndef LED_DEVICE_H
#define LED_DEVICE_H

#include <atomic>
#include "devices/ControllableTaskDevice.h"

class LedDevice : public ControllableTaskDevice
{
public:
    static constexpr unsigned long DEFAULT_BLINK_TIME = 500;

    enum class Mode
    {
        OFF,
        ON,
        BLINKING
    };

    LedDevice(const String &id, NotifyClients callback = nullptr);

    void getConfigFromJson(const JsonDocument &config) override;

    void addStateToJson(JsonDocument &doc) override;
    void addConfigToJson(JsonDocument &doc) const override;
    bool control(const String &action, JsonObject *args = nullptr) override;

    std::vector<int> getPins() const override;

    /**
     * @brief Set LED to static ON or OFF
     * @param state true for ON, false for OFF
     */
    void set(bool state);

    /**
     * @brief Blink the LED
     * @param onTime Time in ms to stay ON
     * @param offTime Time in ms to stay OFF
     */
    void blink(unsigned long onTime, unsigned long offTime);

    /**
     * @brief Get current mode
     * @return Current LED mode
     */
    Mode mode() const { return _desiredMode; }

protected:
    void task() override;

    // Configuration
    String _name;
    int _pin = -1;

    // Thread-safe communication variables
    std::atomic<Mode> _desiredMode{Mode::OFF};
    std::atomic<bool> _desiredState{false};
    std::atomic<unsigned long> _blinkOnDurationMs{DEFAULT_BLINK_TIME};
    std::atomic<unsigned long> _blinkOffDurationMs{DEFAULT_BLINK_TIME};

    // Internal state
    bool _isOn = false;
};

#endif // LED_DEVICE_H
