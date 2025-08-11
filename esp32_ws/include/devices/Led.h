/**
 * @file Led.h
 * @brief LED control class for marble track system
 *
 * This class provides LED control functionality with setup and loop operations
 * for managing LED states in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

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
     * @param pin GPIO pin number for the LED
     * @param id Unique identifier string for the LED
     * @param name Human-readable name string for the LED
     */
    Led(int pin, const String &id, const String &name);

    // Device interface implementation
    String getId() const override { return _id; }
    void loop() override {} // No periodic operations needed

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    std::vector<int> getPins() const override { return {_pin}; }

    // LED-specific operations
    void set(bool state);
    void toggle(); // Toggle LED state

private:
    int _pin;             ///< GPIO pin number for the LED
    String _id;           ///< Unique identifier string for the LED
    String _mode;         ///< Current mode of the LED
};

#endif // LED_H
