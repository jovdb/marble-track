/**
 * @file Servo.cpp
 * @brief Implementation of ServoDevice control class using ESP32 AnalogWrite
 *
 * This file contains the implementation of the ServoDevice class methods
 * for controlling servo motors in the marble track system with smooth,
 * speed-controlled movement.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Servo.h"
#include <pwmWrite.h>

ServoDevice::ServoDevice(int pin, const String &id, const String &name, int initialAngle, int pwmChannel)
    : _pin(pin), _id(id), _name(name), _currentAngle(constrainAngle(initialAngle)), 
      _targetAngle(constrainAngle(initialAngle)), _speed(60.0), _isMoving(false), 
      _lastUpdate(0), _pwmChannel(pwmChannel), _servoPwm()
{
    Serial.println("Servo [" + _id + "]: Created on pin " + String(_pin) + ", PWM channel " + String(_pwmChannel));
}

void ServoDevice::setup()
{
    _servoPwm.attachServo(_pin, _pwmChannel);
    _servoPwm.writeServo(_pin, _currentAngle);
    _lastUpdate = millis();
    Serial.println("Servo [" + _id + "]: Setup complete at angle " + String(_currentAngle) + " with speed " + String(_speed) + "°/s");
}

ServoDevice::~ServoDevice()
{
    // Cleanup handled by ESP32 AnalogWrite library
}

void ServoDevice::loop()
{
    if (_isMoving)
    {
        updateMovement();
    }
}

void ServoDevice::setAngle(int angle)
{
    setAngle(angle, _speed);
}

void ServoDevice::setAngle(int angle, float speed)
{
    int newAngle = constrainAngle(angle);
    if (newAngle != _targetAngle)
    {
        _targetAngle = newAngle;
        _speed = max(1.0f, speed); // Minimum speed of 1 degree per second
        _isMoving = (_currentAngle != _targetAngle);
        _lastUpdate = millis();
        
        if (!_isMoving)
        {
            // If already at target, write immediately
            _servoPwm.writeServo(_pin, _currentAngle);
        }
        
        Serial.println("Servo [" + _id + "]: Moving from " + String(_currentAngle) + "° to " + String(_targetAngle) + "° at " + String(_speed) + "°/s");
        notifyStateChange();
    }
}

void ServoDevice::setSpeed(float speed)
{
    _speed = max(1.0f, speed); // Minimum speed of 1 degree per second
    Serial.println("Servo [" + _id + "]: Speed set to " + String(_speed) + "°/s");
}

void ServoDevice::updateMovement()
{
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - _lastUpdate;
    
    if (deltaTime >= 20) // Update every 20ms for smooth movement
    {
        // Calculate how much to move based on speed and time elapsed
        float degreesToMove = (_speed * deltaTime) / 1000.0; // Convert ms to seconds
        
        int previousAngle = _currentAngle;
        
        if (_currentAngle < _targetAngle)
        {
            _currentAngle = min(_targetAngle, (int)(_currentAngle + degreesToMove));
        }
        else if (_currentAngle > _targetAngle)
        {
            _currentAngle = max(_targetAngle, (int)(_currentAngle - degreesToMove));
        }
        
        // Only write to servo if angle actually changed
        if (_currentAngle != previousAngle)
        {
            _servoPwm.writeServo(_pin, _currentAngle);
            Serial.println("Servo [" + _id + "]: Moving to " + String(_currentAngle) + "° (target: " + String(_targetAngle) + "°)");
        }
        
        // Check if movement is complete
        if (_currentAngle == _targetAngle)
        {
            _isMoving = false;
            Serial.println("Servo [" + _id + "]: Reached target angle " + String(_currentAngle) + "°");
            notifyStateChange();
        }
        
        _lastUpdate = currentTime;
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
        
        int angle = (*payload)["angle"].as<int>();
        
        if ((*payload)["speed"].is<float>())
        {
            float speed = (*payload)["speed"].as<float>();
            setAngle(angle, speed);
        }
        else
        {
            setAngle(angle);
        }
        return true;
    }
    else if (action == "setSpeed")
    {
        if (!payload || !(*payload)["speed"].is<float>())
        {
            Serial.println("Servo [" + _id + "]: Missing speed parameter");
            return false;
        }
        setSpeed((*payload)["speed"].as<float>());
        return true;
    }
    else if (action == "stop")
    {
        _targetAngle = _currentAngle;
        _isMoving = false;
        Serial.println("Servo [" + _id + "]: Movement stopped at angle " + String(_currentAngle) + "°");
        notifyStateChange();
        return true;
    }
    
    Serial.println("Servo [" + _id + "]: Unknown action: " + action);
    return false;
}

String ServoDevice::getState()
{
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;
    doc["angle"] = _currentAngle;
    doc["targetAngle"] = _targetAngle;
    doc["speed"] = _speed;
    doc["isMoving"] = _isMoving;
    doc["pwmChannel"] = _pwmChannel;
    String result;
    serializeJson(doc, result);
    return result;
}

int ServoDevice::constrainAngle(int angle)
{
    return constrain(angle, 0, 180);
}
