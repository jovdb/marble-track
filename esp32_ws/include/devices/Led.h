#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Device.h"

/**
 * @class Led
 * @brief Simple LED control class
 *
 * Provides basic LED control functionality with automatic initialization.
 */
class Led : public Device
{

    enum class LedMode
    {
        BLINKING, ///< LED is blinking
        ON,       ///< LED is turned on
        OFF       ///< LED is turned off
    };

public:
    /**
     * @brief Constructor - automatically initializes pin
     * @param id Unique identifier string for the LED
     * @param name Human-readable name string for the LED
     */
    Led(const String &id, const String &name);
    void setup();
    void loop();

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    JsonObject getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override { return _pin != -1 ? std::vector<int>{_pin} : std::vector<int>{}; }

    // LED-specific operations
    void set(bool state);
    void toggle(); // Toggle LED state

    // Non-blocking blink
    void blink(unsigned long onTime = 500, unsigned long offTime = 500);

    // Configuration
    void configurePin(int pin);
    bool isConfigured() const { return _pin != -1; }

private:
    int _pin = -1;        ///< GPIO pin number for the LED
    LedMode _mode = LedMode::OFF; ///< Current mode of the LED
    // Blink state
    unsigned long _blinkOnTime = 500;
    unsigned long _blinkOffTime = 500;
    unsigned long _lastToggleTime = 0;
    bool _isOn = false;
};

#endif // LED_H
