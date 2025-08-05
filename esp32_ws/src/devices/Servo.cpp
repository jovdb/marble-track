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
    : _pin(pin), _id(id), _name(name), _currentAngle(constrainAngle(initialAngle))
{
    Serial.println("Servo [" + _id + "]: Created on pin " + String(_pin));
}

void ServoDevice::setup()
{
    myservo.attach(_pin);
    setAngle(_currentAngle);
    Serial.println("Servo [" + _id + "]: Setup complete at angle " + String(_currentAngle));
}

ServoDevice::~ServoDevice()
{
    // Cleanup handled by library
}

void ServoDevice::setAngle(int angle)
{
    int newAngle = constrainAngle(angle);
    if (newAngle != _currentAngle)
    {
        _currentAngle = newAngle;
        myservo.write(_pin, _currentAngle);
        Serial.println("Servo [" + _id + "]: Set to angle " + String(_currentAngle));
        notifyStateChange();
    }
}

bool ServoDevice::control(const String &action, JsonObject *payload)
{
    if (action == "setAngle")
    {
        if (!payload || !(*payload)["angle"].is<int>())
        {
            Serial.println("Servo [" + _id + "]: Missing angle parameter");
            return false;
        }
        setAngle((*payload)["angle"].as<int>());
        return true;
    }
    
    Serial.println("Servo [" + _id + "]: Unknown action: " + action);
    return false;
}

String ServoDevice::getState()
{
    JsonDocument doc;
    doc["angle"] = _currentAngle;
    doc["pin"] = _pin;
    doc["name"] = _name;
    doc["type"] = _type;
    String result;
    serializeJson(doc, result);
    return result;
}

int ServoDevice::constrainAngle(int angle)
{
    return constrain(angle, 0, 180);
}
