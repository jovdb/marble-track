/**
 * @file Servo.cpp
 * @brief Implementation of ServoDevice control class using ESP32 AnalogWrite
 *
 * This file contains the implementation of the ServoDevice class methods
 * for controlling servo motors in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Servo.h"
#include <Servo.h>

// Global servo instance for ESP32 AnalogWrite library
Servo myservo;

ServoDevice::ServoDevice(int pin, const String &id, const String &name, int initialAngle)
    : pin(pin), id(id), name(name), currentAngle(constrainAngle(initialAngle))
{
    Serial.println("Servo [" + id + "]: Created on pin " + String(pin));
}

void ServoDevice::setup()
{
    myservo.attach(pin);
    setAngle(currentAngle);
    Serial.println("Servo [" + id + "]: Setup complete at angle " + String(currentAngle));
}

ServoDevice::~ServoDevice()
{
    // Cleanup handled by library
}

void ServoDevice::setAngle(int angle)
{
    int newAngle = constrainAngle(angle);
    if (newAngle != currentAngle)
    {
        currentAngle = newAngle;
        myservo.write(pin, currentAngle);
        Serial.println("Servo [" + id + "]: Set to angle " + String(currentAngle));
        notifyStateChange();
    }
}

bool ServoDevice::control(const String &action, JsonObject *payload)
{
    if (action == "setAngle")
    {
        if (!payload || !(*payload)["angle"].is<int>())
        {
            Serial.println("Servo [" + id + "]: Missing angle parameter");
            return false;
        }
        setAngle((*payload)["angle"].as<int>());
        return true;
    }
    
    Serial.println("Servo [" + id + "]: Unknown action: " + action);
    return false;
}

String ServoDevice::getState()
{
    JsonDocument doc;
    doc["angle"] = currentAngle;
    String result;
    serializeJson(doc, result);
    return result;
}

int ServoDevice::constrainAngle(int angle)
{
    return constrain(angle, 0, 180);
}
