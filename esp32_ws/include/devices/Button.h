/**
 * @file Button.h
 * @brief Button input class for marble track system
 *
 * This class provides button input functionality with debouncing, state tracking,
 * and event detection for managing button interactions in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Device.h"

/**
 * @class Button
 * @brief Button input control class with debouncing
 *
 * Provides button input functionality with automatic debouncing, state tracking,
 * and event detection. Supports both pull-up and pull-down configurations.
 */
class Button : public Device
{
public:
    /**
     * @brief Constructor for Button class
     * @param pin GPIO pin number for the button
     * @param id Unique identifier string for the button
     * @param name Human-readable name string for the button
     * @param pullUp true for internal pull-up (button connects to ground), false for pull-down
     * @param debounceMs Debounce time in milliseconds (default: 50ms)
     */
    Button(int pin, const String &id, const String &name, bool pullUp = true, unsigned long debounceMs = 50);

    /**
     * @brief Setup function to initialize the button
     * Must be called in setup() before using the button
     */
    void setup();

    // Device interface implementation
    String getId() const override { return id; }
    String getName() const override { return name; }
    String getType() const override { return type; }
    void loop() override; // Handles debouncing and state tracking

    String getState() override;

    // Button-specific operations
    bool isPressed() const { return currentState; }
    bool wasPressed(); // Returns true once when button is pressed (edge detection)
    bool wasReleased(); // Returns true once when button is released (edge detection)
    unsigned long getPressedTime() const; // How long button has been pressed (ms)

private:
    int pin;                    ///< GPIO pin number for the button
    String id;                  ///< Unique identifier string for the button
    String name;                ///< Human-readable name string for the button
    String type = "BUTTON";     ///< Type of the device
    bool pullUp;                ///< Pull-up configuration (true = internal pull-up)
    unsigned long debounceMs;   ///< Debounce time in milliseconds

    // State tracking
    bool currentState = false;          ///< Current debounced state
    bool rawState = false;              ///< Raw pin reading

    // Timing
    unsigned long lastDebounceTime = 0; ///< Last time the pin state changed
    unsigned long pressStartTime = 0;   ///< When the current press started
    
    // Edge detection flags
    bool pressedFlag = false;           ///< Set when button is pressed (cleared by wasPressed())
    bool releasedFlag = false;          ///< Set when button is released (cleared by wasReleased())

    /**
     * @brief Read the raw pin state accounting for pull-up/pull-down configuration
     * @return true if button is physically pressed, false otherwise
     */
    bool readRawState();
};

#endif // BUTTON_H
