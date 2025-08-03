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
#include "IControllable.h"

/**
 * @class Led
 * @brief LED control and management class
 * 
 * Provides functionality to control LEDs with setup initialization,
 * continuous loop operations, and state control.
 * Implements IControllable interface for unified device control.
 */
class Led : public IControllable {
public:
    /**
     * @brief Constructor for Led class
     * @param pin GPIO pin number for the LED
     * @param id Unique identifier string for the LED
     * @param name Human-readable name string for the LED
     */
    Led(int pin, const String& id, const String& name);
    
    /**
     * @brief Destructor for Led class
     */
    ~Led() override;
    
    // IControllable interface implementation
    /**
     * @brief Dynamic control function for LED operations
     * @param action The action to perform (e.g., "set")
     * @param payload Pointer to JSON object containing action parameters (can be nullptr)
     * @return true if action was successful, false otherwise
     */
    bool control(const String& action, JsonObject* payload = nullptr) override;
    
    /**
     * @brief Get device identifier
     * @return String identifier of the LED
     */
    String getId() const override;
    
    // Led-specific public methods
    /**
     * @brief Setup function to initialize LED hardware and configurations
     * 
     * This function should be called once during system initialization
     * to configure LED pins and initial states.
     */
    void setup();
    
    /**
     * @brief Loop function for continuous LED operations
     * 
     * This function should be called repeatedly in the main loop.
     * Currently empty but ready for future LED operations.
     */
    void loop();
    
    /**
     * @brief Set LED state
     * @param state true to turn LED on, false to turn LED off
     */
    void set(bool state);

private:
    int pin;           ///< GPIO pin number for the LED
    String id;         ///< Unique identifier string for the LED
    String name;       ///< Human-readable name string for the LED
    bool currentState; ///< Current LED state (on/off)
};

#endif // LED_H
