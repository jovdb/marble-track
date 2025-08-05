/**
 * @file Servo.cpp
 * @brief Implementation of ServoDevice control class using ESP32 AnalogWrite library
 *
 * This file contains the implementation of the ServoDevice class methods
 * for controlling servo motors in the marble track system using AnalogWrite.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Servo.h"
#include <Servo.h>

// Global servo instance for ESP32 AnalogWrite library
Servo myServo;

/**
 * @brief Constructor for ServoDevice class
 *
 * Initializes the ServoDevice object with pin, id, name, and initial angle.
 * Hardware initialization is done separately in setup() function.
 * @param pin GPIO pin number for the servo
 * @param id Unique identifier string for the servo
 * @param name Human-readable name string for the servo
 * @param initialAngle Initial angle for the servo (0-180 degrees)
 */
ServoDevice::ServoDevice(int pin, const String &id, const String &name, int initialAngle)
    : pin(pin), id(id), name(name), currentAngle(constrainAngle(initialAngle))
{
    Serial.println("Servo [" + id + "]: Created on pin " + String(pin) + " with initial angle " + String(currentAngle));
}

/**
 * @brief Setup servo hardware and initialize PWM
 * 
 * Configures PWM for servo control and sets initial position.
 * Call this after constructor to initialize the servo hardware.
 */
void ServoDevice::setup()
{
    // Attach the servo to the pin
    myServo.attach(pin);
    
    // Set initial angle
    setAngle(currentAngle);
    
    Serial.println("Servo [" + id + "]: Setup complete on pin " + String(pin) + " at angle " + String(currentAngle));
}

/**
 * @brief Destructor for ServoDevice class
 */
ServoDevice::~ServoDevice()
{
    // No cleanup needed for AnalogWrite
}

/**
 * @brief Set servo angle
 * @param angle Desired angle (0-180 degrees)
 */
void ServoDevice::setAngle(int angle)
{
    int constrainedAngle = constrainAngle(angle);
    
    if (constrainedAngle != currentAngle)
    {
        currentAngle = constrainedAngle;
        
        // Use ESP32 AnalogWrite library's Servo.h API
        myServo.write(pin, currentAngle);
        
        Serial.println("Servo [" + id + "]: Set to angle " + String(currentAngle));
        
        // Notify state change for real-time updates
        notifyStateChange();
    }
}

/**
 * @brief Dynamic control function for servo operations
 * @param action The action to perform (e.g., "setAngle")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool ServoDevice::control(const String &action, JsonObject *payload)
{
    if (action == "setAngle")
    {
        if (!payload || !(*payload)["angle"].is<int>())
        {
            Serial.println("Servo [" + id + "]: Invalid 'setAngle' payload - angle required");
            return false;
        }
        setAngle((*payload)["angle"].as<int>());
    }
    else if (action == "home")
    {
        setAngle(90); // Return to center position
    }
    else if (action == "min")
    {
        setAngle(0); // Move to minimum position
    }
    else if (action == "max")
    {
        setAngle(180); // Move to maximum position
    }
    else
    {
        Serial.println("Servo [" + id + "]: Unknown action: " + action);
        return false;
    }

    return true;
}

/**
 * @brief Get current state of the servo
 * @return String containing JSON representation of the current state
 */
String ServoDevice::getState()
{
    JsonDocument doc;
    doc["angle"] = currentAngle;
    doc["position"] = currentAngle; // Alias for compatibility
    
    String result;
    serializeJson(doc, result);
    return result;
}

/**
 * @brief Validate and constrain angle to valid range
 * @param angle Input angle to validate
 * @return Constrained angle (0-180)
 */
int ServoDevice::constrainAngle(int angle)
{
    return constrain(angle, 0, 180);
}
