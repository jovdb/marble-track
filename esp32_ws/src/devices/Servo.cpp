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
    : Device(id, name, "SERVO"), _pin(pin), _currentAngle(constrainAngle(initialAngle)), 
      _targetAngle(constrainAngle(initialAngle)), _speed(60.0), _isMoving(false), 
      _lastUpdate(0), _pwmChannel(pwmChannel), _servoPwm()
{
        log("Created on pin %d, PWM channel %d\n", _pin, _pwmChannel);
}

void ServoDevice::setup()
{
    _servoPwm.attachServo(_pin, _pwmChannel);
    _servoPwm.writeServo(_pin, _currentAngle);
    _lastUpdate = millis();
    log("Setup complete at angle %d with speed %.2f°/s\n", _currentAngle, _speed);
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
        
    log("Moving from %d° to %d° at %.2f°/s\n", _currentAngle, _targetAngle, _speed);
        notifyStateChange();
    }
}

void ServoDevice::setSpeed(float speed)
{
    _speed = max(1.0f, speed); // Minimum speed of 1 degree per second
    log("Speed set to %.2f°/s\n", _speed);
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
            log("Moving to %d° (target: %d°)\n", _currentAngle, _targetAngle);
        }
        
        // Check if movement is complete
        if (_currentAngle == _targetAngle)
        {
            _isMoving = false;
            log("Reached target angle %d°\n", _currentAngle);
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
        log("Missing angle parameter\n");
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
        log("Missing speed parameter\n");
            return false;
        }
        setSpeed((*payload)["speed"].as<float>());
        return true;
    }
    else if (action == "stop")
    {
        _targetAngle = _currentAngle;
        _isMoving = false;
    log("Movement stopped at angle %d°\n", _currentAngle);
        notifyStateChange();
        return true;
    }
    
    log("Unknown action: %s\n", action.c_str());
    return false;
}

String ServoDevice::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>()) {
        doc[kv.key()] = kv.value();
    }
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
