/**
 * @file Led.cpp
 * @brief Implementation of LED control class
 *
 * This file contains the implementation of the Led class methods
 * for controlling LEDs in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Led.h"

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with pin, id, and name parameters.
 * @param pin GPIO pin number for the LED
 * @param id Unique identifier string for the LED
 * @param name Human-readable name string for the LED
 */
Led::Led(int pin, const String &id, const String &name)
    : pin(pin), id(id), name(name), mode("OFF")
{
    // Initialize the pin as output and set initial state
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    Serial.println("Led [" + id + "]: Initialized on pin " + String(pin));
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 */
void Led::set(bool state)
{
    digitalWrite(pin, state ? HIGH : LOW);
    mode = state ? "ON" : "OFF";
    Serial.println("Led [" + id + "]: " + mode);
    
    // Notify state change
    JsonObject currentState = getState();
    notifyStateChange(currentState);
}

/**
 * @brief Dynamic control function for LED operations
 * @param action The action to perform (e.g., "set")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Led::control(const String &action, JsonObject *payload)
{
    Serial.println("Led [" + id + "]: " + action);

    // Simple action mapping
    if (action == "on")
    {
        set(true);
    }
    else if (action == "off")
    {
        set(false);
    }
    else if (action == "toggle")
    {
        set(!mode.equals("ON"));
    }
    else if (action == "set")
    {
        if (!payload || !(*payload)["state"].is<bool>())
        {
            Serial.println("Led [" + id + "]: Invalid 'set' payload");
            return false;
        }
        set((*payload)["state"].as<bool>());
    }
    else
    {
        Serial.println("Led [" + id + "]: Unknown action: " + action);
        return false;
    }

    return true;
}

/**
 * @brief Get current state of the LED
 * @return JsonObject containing the current mode of the LED
 */
JsonObject Led::getState()
{
    JsonDocument doc;
    JsonObject state = doc.to<JsonObject>();
    state["mode"] = mode;
    return state;
}
