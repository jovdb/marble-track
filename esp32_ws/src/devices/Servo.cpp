#include "esp_log.h"

static const char *TAG = "Servo";
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
    : Device(id, name, "servo"), _pin(pin), _currentAngle(constrainAngle(initialAngle)),
      _targetAngle(constrainAngle(initialAngle)), _speed(60.0), _isMoving(false),
      _lastUpdate(0), _pwmChannel(pwmChannel), _servoPwm()
{
    ESP_LOGI(TAG, "Servo [%s]: Created on pin %d, PWM channel %d", _id.c_str(), _pin, _pwmChannel);
}

void ServoDevice::setup()
{
    _servoPwm.attachServo(_pin, _pwmChannel);
    _servoPwm.writeServo(_pin, _currentAngle);
    _lastUpdate = millis();
    ESP_LOGI(TAG, "Servo [%s]: Setup complete at angle %d with speed %.2f deg/s", _id.c_str(), _currentAngle, _speed);
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

        ESP_LOGI(TAG, "Servo [%s]: Moving from %d deg to %d deg at %.2f deg/s", _id.c_str(), _currentAngle, _targetAngle, _speed);
        notifyStateChange();
    }
}

void ServoDevice::setSpeed(float speed)
{
    _speed = max(1.0f, speed); // Minimum speed of 1 degree per second
    ESP_LOGI(TAG, "Servo [%s]: Speed set to %.2f deg/s", _id.c_str(), _speed);
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
            ESP_LOGI(TAG, "Servo [%s]: Moving to %d deg (target: %d deg)", _id.c_str(), _currentAngle, _targetAngle);
        }

        // Check if movement is complete
        if (_currentAngle == _targetAngle)
        {
            _isMoving = false;
            ESP_LOGI(TAG, "Servo [%s]: Reached target angle %d deg", _id.c_str(), _currentAngle);
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
            ESP_LOGE(TAG, "Servo [%s]: Missing angle parameter", _id.c_str());
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
            ESP_LOGE(TAG, "Servo [%s]: Missing speed parameter", _id.c_str());
            return false;
        }
        setSpeed((*args)["speed"].as<float>());
        return true;
    }
    else if (action == "stop")
    {
        _targetAngle = _currentAngle;
        _isMoving = false;
        ESP_LOGI(TAG, "Servo [%s]: Movement stopped at angle %d deg", _id.c_str(), _currentAngle);
        notifyStateChange();
        return true;
    }

    ESP_LOGW(TAG, "Servo [%s]: Unknown action: %s", _id.c_str(), action.c_str());
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
