/**
 * @file Led.h
 * @brief LED control class for marble track system
 * 
 * This class provides LED control functionality with setup and loop operations
 * for managing LED states, patterns, and animations in the marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef LED_H
#define LED_H

#include <Arduino.h>

/**
 * @class Led
 * @brief LED control and management class
 * 
 * Provides functionality to control LEDs with setup initialization
 * and continuous loop operations for patterns and animations.
 */
class Led {
public:
    // Readonly public members
    const int pin;        ///< GPIO pin number for the LED
    const String id;      ///< Unique string identifier for the LED
    const String name;    ///< Human-readable name for the LED

    /**
     * @brief Constructor for Led class
     * @param pin GPIO pin number for the LED
     * @param id Unique string identifier for the LED
     * @param name Human-readable name for the LED
     */
    Led(int pin, const String& id, const String& name);
    
    /**
     * @brief Destructor for Led class
     */
    ~Led();
    
    /**
     * @brief Setup function to initialize LED hardware and configurations
     * 
     * This function should be called once during system initialization
     * to configure LED pins, initial states, and any required settings.
     */
    void setup();
    
    /**
     * @brief Loop function for continuous LED operations
     * 
     * This function should be called repeatedly in the main loop
     * to handle LED animations, patterns, and state updates.
     */
    void loop();

private:
    // LED state management
    bool currentState;     ///< Current LED state (on/off)
    unsigned long lastBlink; ///< Last blink timestamp for timing
};

#endif // LED_H
