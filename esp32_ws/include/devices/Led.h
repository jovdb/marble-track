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
public:
    /**
     * @brief Constructor - automatically initializes pin
     * @param id Unique identifier string for the LED
     * @param name Human-readable name string for the LED
     */
    Led(const String &id, const String &name);
    void setup();
    void loop() override {} // No periodic operations needed

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    std::vector<int> getPins() const override { return {_pin}; }

    // LED-specific operations
    void set(bool state);
    void toggle(); // Toggle LED state

private:
    int _pin = -1;        ///< GPIO pin number for the LED
    String _mode = "OFF"; ///< Current mode of the LED
};

#endif // LED_H
