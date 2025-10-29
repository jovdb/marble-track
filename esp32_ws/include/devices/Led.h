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
    Led(const String &id, NotifyClients callback = nullptr);

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
    std::vector<int> getPins() const override;

    // LED-specific operations
    bool set(bool state);
    void toggle(); // Toggle LED state

    // Non-blocking blink
    bool blink(unsigned long onTime = 500, unsigned long offTime = 500);

private:
    int _pin = -1;
    LedMode _mode = LedMode::OFF;
    LedMode _initialMode = LedMode::OFF;

    // Blink state
    unsigned long _blinkOnTime = 500;
    unsigned long _blinkOffTime = 500;
    unsigned long _lastToggleTime = 0;

    // When in blinking mode we need to know if the LED is currently on or off to toggle it
    bool _isOn = false;
    bool _didSetup = false;

    void applyInitialState();
    LedMode modeFromString(const String &value) const;
    String modeToString(LedMode mode) const;
};

#endif // LED_H
