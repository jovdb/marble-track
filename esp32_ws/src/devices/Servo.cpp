#include "Logging.h"
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

ServoDevice::ServoDevice(const String &id, const String &name)
    : Device(id, "servo"), _servoPwm()
{
    _name = name;
    // Defaults set in member initializers
    MLOG_INFO("Servo [%s]: Created (defaults: pin %d, angle %d, pwm %d)", _id.c_str(), _pin, _currentAngle, _pwmChannel);
}

void ServoDevice::setup()
{
    _pin = -1;
    _currentAngle = 0;
    _targetAngle = 0;
    _pwmChannel = 0;
    _servoPwm.attachServo(_pin, _pwmChannel);
    _servoPwm.writeServo(_pin, _currentAngle);
    _lastUpdate = millis();
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

        MLOG_INFO("Servo [%s]: Moving from %d deg to %d deg at %.2f deg/s", _id.c_str(), _currentAngle, _targetAngle, _speed);
        notifyStateChange();
    }
}

void ServoDevice::setSpeed(float speed)
{
    _speed = max(1.0f, speed); // Minimum speed of 1 degree per second
    MLOG_INFO("Servo [%s]: Speed set to %.2f deg/s", _id.c_str(), _speed);
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
            MLOG_INFO("Servo [%s]: Moving to %d deg (target: %d deg)", _id.c_str(), _currentAngle, _targetAngle);
        }

        // Check if movement is complete
        if (_currentAngle == _targetAngle)
        {
            _isMoving = false;
            MLOG_INFO("Servo [%s]: Reached target angle %d deg", _id.c_str(), _currentAngle);
            notifyStateChange();
        }

        _lastUpdate = currentTime;
    }
}

bool ServoDevice::control(const String &action, JsonObject *args)
{
    if (action == "setAngle")
    {
        if (!args || !(*args)["angle"].is<int>())
        {
            MLOG_ERROR("Servo [%s]: Missing angle parameter", _id.c_str());
            return false;
        }

        int angle = (*args)["angle"].as<int>();

        if ((*args)["speed"].is<float>())
        {
            float speed = (*args)["speed"].as<float>();
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
        if (!args || !(*args)["speed"].is<float>())
        {
            MLOG_ERROR("Servo [%s]: Missing speed parameter", _id.c_str());
            return false;
        }
        setSpeed((*args)["speed"].as<float>());
        return true;
    }
    else if (action == "stop")
    {
        _targetAngle = _currentAngle;
        _isMoving = false;
        MLOG_INFO("Servo [%s]: Movement stopped at angle %d deg", _id.c_str(), _currentAngle);
        notifyStateChange();
        return true;
    }

    MLOG_WARN("Servo [%s]: Unknown action: %s", _id.c_str(), action.c_str());
    return false;
}

String ServoDevice::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
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

int ServoDevice::getPin() const { return _pin; }
void ServoDevice::setPin(int pin) { _pin = pin; }
int ServoDevice::getPwmChannel() const { return _pwmChannel; }
void ServoDevice::setPwmChannel(int channel) { _pwmChannel = channel; }
