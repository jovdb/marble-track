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
#include "IDevice.h"
#include "../IControllable.h"

/**
 * @class Led
 * @brief Simple LED control class
 * 
 * Provides basic LED control functionality with automatic initialization.
 * Implements IDevice and IControllable interfaces for system integration.
 */
class Led : public IDevice, public IControllable {
public:
    /**
     * @brief Constructor - automatically initializes pin
     * @param pin GPIO pin number for the LED
     * @param id Unique identifier string for the LED
     * @param name Human-readable name string for the LED
     */
    Led(int pin, const String& id, const String& name);
    
    // IDevice interface implementation
    String getId() const override { return id; }
    String getName() const override { return name; }
    void loop() override {} // No periodic operations needed
    
    // IControllable interface support
    bool supportsControllable() const override { return true; }
    IControllable* getControllableInterface() override { return this; }
    bool control(const String& action, JsonObject* payload = nullptr) override;
    
    // LED-specific operations
    void set(bool state);

private:
    int pin;           ///< GPIO pin number for the LED
    String id;         ///< Unique identifier string for the LED
    String name;       ///< Human-readable name string for the LED
    bool currentState; ///< Current LED state (on/off)
};

#endif // LED_H
