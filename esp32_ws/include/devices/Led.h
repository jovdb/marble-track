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
     * @brief Constructor - automatically initializes with ID only
     * @param id Unique identifier string for the LED
     */
    Led(const String &id);

    // Name getter and setter
    String getName() const { return _name; }
    void setName(const String &name) { _name = name; }
    void setup();
    void loop();

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override { return _pin != -1 ? std::vector<int>{_pin} : std::vector<int>{}; }

    // LED-specific operations
    void set(bool state);
    void toggle(); // Toggle LED state

    // Non-blocking blink
    void blink(unsigned long onTime = 500, unsigned long offTime = 500);

private:
    int _pin = -1;
    LedMode _mode = LedMode::OFF;
    // Blink state
    unsigned long _blinkOnTime = 500;
    unsigned long _blinkOffTime = 500;
    unsigned long _lastToggleTime = 0;
    bool _isOn = false;
};

#endif // LED_H
