/**
 * @file LedDevice.h
 * @brief LED device implementation using TaskDevice
 */

#ifndef LED_DEVICE_H
#define LED_DEVICE_H

#include "devices/ControllableTaskDevice.h"

class LedDevice : public ControllableTaskDevice
{
public:
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

protected:
    void task() override;

private:
    static const unsigned long DEFAULT_BLINK_TIME = 500;  // Default blink on/off time in ms
    uint8_t _pin;
    String _name;

    enum class Mode
    {
        OFF,
        ON,
        BLINKING
    };

    // Thread-safe communication variables
    volatile Mode _targetMode = Mode::OFF;
    volatile bool _targetState = false;
    volatile unsigned long _targetBlinkOnTime = DEFAULT_BLINK_TIME;
    volatile unsigned long _targetBlinkOffTime = DEFAULT_BLINK_TIME;

    // Internal state
    bool _isOn = false;
};

#endif // LED_DEVICE_H
