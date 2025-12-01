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

    void updateConfig(const JsonDocument &config) override;
    JsonDocument getConfig() const override;

    void addStateToJson(JsonDocument &doc) override;
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
    volatile unsigned long _targetBlinkOnTime = 500;
    volatile unsigned long _targetBlinkOffTime = 500;

    // Internal state
    bool _isOn = false;
};

#endif // LED_DEVICE_H
