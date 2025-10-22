/**
 * @file Servo.h
 * @brief Servo control class for marble track system
 *
 * This class provides servo control functionality with angle management
 * for controlling servo motors in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef SERVO_H
#define SERVO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Device.h"
#include <pwmWrite.h>

/**
 * @class ServoDevice
 * @brief Servo motor control class
 *
 * Provides servo motor control functionality with angle management and
 * automatic initialization. Supports angle range 0-180 degrees with
 * configurable movement speed for smooth transitions.
 */
class ServoDevice : public Device
{
public:
    // Pin and PWM channel getters/setters
    int getPin() const;
    void setPin(int pin);
    int getPwmChannel() const;
    void setPwmChannel(int channel);
    /**
     * @brief Constructor - creates servo object without initializing hardware
     * @param id Unique identifier string for the servo
     * @param name Human-readable name string for the servo
     */
    ServoDevice(const String &id, const String &name);

    /**
     * @brief Destructor - cleans up servo object
     */
    virtual ~ServoDevice();

    /**
     * @brief Setup servo hardware and initialize PWM
     * Call this after constructor to initialize the servo hardware
     */
    void setup(); // Now sets up pin, initialAngle, pwmChannel with defaults

    void loop() override; // Handle smooth movement updates

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    std::vector<int> getPins() const override { return {_pin}; }

    // Servo-specific operations
    bool setAngle(int angle);
    bool setAngle(int angle, float speed); // Speed in degrees per second
    bool setSpeed(float speed);            // Set default movement speed
    int getAngle() const { return _currentAngle; }
    int getTargetAngle() const { return _targetAngle; }
    float getSpeed() const { return _speed; }
    bool isMoving() const { return _isMoving; }

private:
    int _pin = 1;                  ///< GPIO pin number for the servo
    int _currentAngle = 90;        ///< Current angle of the servo (0-180)
    int _targetAngle = 90;         ///< Target angle for smooth movement
    float _speed = 60.0;           ///< Movement speed in degrees per second
    bool _isMoving = false;        ///< Flag indicating if servo is currently moving
    unsigned long _lastUpdate = 0; ///< Last time movement was updated
    int _pwmChannel = 0;           ///< PWM channel for ESP32 AnalogWrite
    Pwm _servoPwm;                 ///< Persistent PWM instance for this servo

    /**
     * @brief Validate and constrain angle to valid range
     * @param angle Input angle to validate
     * @return Constrained angle (0-180)
     */
    int constrainAngle(int angle);

    /**
     * @brief Update servo position for smooth movement (non-blocking)
     */
    void updateMovement();
};

#endif // SERVO_H
