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
    String getId() const override { return id; }
    String getName() const override { return name; }
    String getType() const override { return type; }
    void loop() override {} // No periodic operations needed

    // Controllable functionality
    bool isControllable() const override { return true; }
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;

    // LED-specific operations
    void set(bool state);

private:
    int pin;             ///< GPIO pin number for the LED
    String id;           ///< Unique identifier string for the LED
    String name;         ///< Human-readable name string for the LED
    String type = "LED"; ///< Type of the device
    String mode;         ///< Current mode of the LED
};

#endif // LED_H
