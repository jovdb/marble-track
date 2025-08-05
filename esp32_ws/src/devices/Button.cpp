/**
 * @file Button.cpp
 * @brief Implementation of Button input class
 *
 * This file contains the implementation of the Button class methods
 * for handling button input with debouncing in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Button.h"

/**
 * @brief Constructor for Button class
 *
 * Initializes the Button object with pin, id, name, and configuration parameters.
 * @param pin GPIO pin number for the button
 * @param id Unique identifier string for the button
 * @param name Human-readable name string for the button
 * @param pullUp true for internal pull-up (button connects to ground), false for pull-down
 * @param debounceMs Debounce time in milliseconds
 */
Button::Button(int pin, const String &id, const String &name, bool pullUp, unsigned long debounceMs)
    : pin(pin), id(id), name(name), pullUp(pullUp), debounceMs(debounceMs)
{
    Serial.println("Button [" + id + "]: Created on pin " + String(pin) +
                   (pullUp ? " (pull-up)" : " (pull-down)") +
                   ", debounce: " + String(debounceMs) + "ms");
}

/**
 * @brief Setup function to initialize the button
 * Must be called in setup() before using the button
 */
void Button::setup()
{
    // Configure pin mode based on pull-up/pull-down setting
    if (pullUp)
    {
        pinMode(pin, INPUT_PULLUP);
    }
    else
    {
        pinMode(pin, INPUT);
    }

    // Initialize state variables
    rawState = readRawState();
    currentState = rawState;
}

/**
 * @brief Loop function for continuous button operations
 *
 * Handles debouncing logic and state change detection.
 * Should be called repeatedly in the main loop.
 */
void Button::loop()
{
    // Read current raw state
    bool newRawState = readRawState();

    // Check if raw state has changed
    if (newRawState != rawState)
    {
        // Reset debounce timer and update raw state
        lastDebounceTime = millis();
        rawState = newRawState;
    }

    // Check if enough time has passed for debouncing
    if ((millis() - lastDebounceTime) > debounceMs)
    {
        // Update debounced state if it has changed
        if (rawState != currentState)
        {
            bool previousState = currentState;
            currentState = rawState;

            // Set edge detection flags
            if (currentState && !previousState)
            {
                // Button was pressed
                pressedFlag = true;
                pressStartTime = millis();
                Serial.println("Button [" + id + "]: PRESSED");
                notifyStateChange(); // Notify state change
            }
            else if (!currentState && previousState)
            {
                // Button was released
                releasedFlag = true;
                unsigned long pressDuration = millis() - pressStartTime;
                Serial.println("Button [" + id + "]: RELEASED (held for " + String(pressDuration) + "ms)");
                notifyStateChange(); // Notify state change
            }
        }
    }
}

/**
 * @brief Check if button was pressed (edge detection)
 * @return true once when button is pressed, false otherwise
 * @note This function clears the pressed flag, so it only returns true once per press
 */
bool Button::wasPressed()
{
    if (pressedFlag)
    {
        pressedFlag = false;
        return true;
    }
    return false;
}

/**
 * @brief Check if button was released (edge detection)
 * @return true once when button is released, false otherwise
 * @note This function clears the released flag, so it only returns true once per release
 */
bool Button::wasReleased()
{
    if (releasedFlag)
    {
        releasedFlag = false;
        return true;
    }
    return false;
}

/**
 * @brief Get how long the button has been pressed
 * @return Time in milliseconds since button was pressed, 0 if not currently pressed
 */
unsigned long Button::getPressedTime() const
{
    if (currentState)
    {
        return millis() - pressStartTime;
    }
    return 0;
}

/**
 * @brief Get current state of the button
 * @return String containing JSON representation of the current state
 */
String Button::getState()
{
    JsonDocument doc;
    doc["pressed"] = currentState;

    String result;
    serializeJson(doc, result);
    return result;
}

/**
 * @brief Read the raw pin state accounting for pull-up/pull-down configuration
 * @return true if button is physically pressed, false otherwise
 */
bool Button::readRawState()
{
    bool pinState = digitalRead(pin);

    // Invert logic for pull-up configuration (pin is LOW when button is pressed)
    if (pullUp)
    {
        return !pinState;
    }
    else
    {
        return pinState;
    }
}
