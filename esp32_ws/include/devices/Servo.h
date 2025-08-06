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
    /**
     * @brief Constructor - creates servo object without initializing hardware
     * @param pin GPIO pin number for the servo
     * @param id Unique identifier string for the servo
     * @param name Human-readable name string for the servo
     * @param initialAngle Initial angle for the servo (0-180 degrees)
     * @param pwmChannel PWM channel for ESP32 AnalogWrite (0-15)
     */
    ServoDevice(int pin, const String &id, const String &name, int initialAngle = 90, int pwmChannel = 0);

    /**
     * @brief Destructor - cleans up servo object
     */
    virtual ~ServoDevice();

    /**
     * @brief Setup servo hardware and initialize PWM
     * Call this after constructor to initialize the servo hardware
     */
    void setup();

    // Device interface implementation
    String getId() const override { return _id; }
    String getName() const override { return _name; }
    String getType() const override { return _type; }
    void loop() override; // Handle smooth movement updates

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;

    // Servo-specific operations
    void setAngle(int angle);
    void setAngle(int angle, float speed); // Speed in degrees per second
    void setSpeed(float speed); // Set default movement speed
    int getAngle() const { return _currentAngle; }
    int getTargetAngle() const { return _targetAngle; }
    float getSpeed() const { return _speed; }
    bool isMoving() const { return _isMoving; }

private:
    int _pin;                    ///< GPIO pin number for the servo
    String _id;                  ///< Unique identifier string for the servo
    String _name;                ///< Human-readable name string for the servo
    String _type = "SERVO";      ///< Type of the device
    int _currentAngle;           ///< Current angle of the servo (0-180)
    int _targetAngle;            ///< Target angle for smooth movement
    float _speed;                ///< Movement speed in degrees per second
    bool _isMoving;              ///< Flag indicating if servo is currently moving
    unsigned long _lastUpdate;   ///< Last time movement was updated
    int _pwmChannel;             ///< PWM channel for ESP32 AnalogWrite
    Pwm _servoPwm;               ///< Persistent PWM instance for this servo

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
