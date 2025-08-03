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

#include "Led.h"

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with pin, id, and name parameters.
 * @param pin GPIO pin number for the LED
 * @param id Unique identifier string for the LED
 * @param name Human-readable name string for the LED
 */
Led::Led(int pin, const String &id, const String &name)
    : pin(pin), id(id), name(name), currentState(false)
{
    // Constructor initializes member variables
    // Pin setup is done in setup() function
}

/**
 * @brief Destructor for Led class
 *
 * Cleans up any resources used by the Led object.
 */
Led::~Led()
{
    // Clean up any allocated resources
    // For simple LED control, usually no cleanup is needed
}

/**
 * @brief Setup function to initialize LED hardware and configurations
 *
 * Configures LED pin as output and sets initial state.
 * This function should be called once during system initialization.
 */
void Led::setup()
{
    Serial.println("Led: Initializing LED [" + id + "] - " + name + " on pin " + String(pin));

    // Initialize the pin as output
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Start with LED off
    currentState = false;

    Serial.println("Led: LED [" + id + "] initialized successfully");
}

/**
 * @brief Loop function for continuous LED operations
 *
 * This function should be called repeatedly in the main loop.
 * Currently empty but ready for future LED operations.
 */
void Led::loop()
{
    // Loop operations can be added here in the future
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 */
void Led::set(bool state)
{
    digitalWrite(pin, state ? HIGH : LOW);
    currentState = state;

    // Log the state change with LED ID
    Serial.println("Led [" + id + "]: Set to " + (state ? "ON" : "OFF"));
}

/**
 * @brief Dynamic control function for LED operations
 * @param action The action to perform (e.g., "set")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Led::control(const String &action, JsonObject *payload)
{
    Serial.println("Led [" + id + "]: Control action '" + action + "' with " + 
                   (payload ? "JSON payload" : "no payload"));

    if (action == "set")
    {
        // Check if payload is provided and contains "state" field
        if (payload && (*payload)["state"].is<bool>())
        {
            bool state = (*payload)["state"].as<bool>();
            set(state);
            return true;
        }
        else if (!payload)
        {
            Serial.println("Led [" + id + "]: Error - Missing payload for 'set' action");
            return false;
        }
        else
        {
            Serial.println("Led [" + id + "]: Error - Missing 'state' field in payload for 'set' action");
            return false;
        }
    }
    else if (action == "toggle")
    {
        // Toggle action doesn't require a payload
        set(!currentState);
        return true;
    }
    else
    {
        Serial.println("Led [" + id + "]: Error - Unknown action: " + action);
        return false;
    }
}

/**
 * @brief Get device identifier (IControllable interface implementation)
 * @return String identifier of the LED
 */
String Led::getId() const
{
    return id;
}
