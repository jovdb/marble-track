/**
 * @file Stepper.h
 * @brief Stepper motor control class for marble track system
 *
 * This class provides stepper motor control functionality with acceleration
 * and precise positioning for controlling stepper motors in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef STEPPER_H
#define STEPPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include "Device.h"

/**
 * @class Stepper
 * @brief Stepper motor control class with acceleration and positioning
 *
 * Provides stepper motor control functionality with acceleration, deceleration,
 * and precise positioning using the AccelStepper library.
 * Supports both 2-pin (DRIVER for NEMA 17) and 4-pin (HALF4WIRE for 28BYJ-48) configurations.
 */
class Stepper : public Device
{
public:
    /**
     * @brief Get all pin numbers used by this stepper
     * @return std::vector<int> of pin numbers
     */
    std::vector<int> getPins() const override;
    /**
     * @brief Constructor for 2-pin stepper (DRIVER - NEMA 17 with driver)
     * @param stepPin GPIO pin number for the step signal
     * @param dirPin GPIO pin number for the direction signal
     * @param id Unique identifier string for the stepper
     * @param name Human-readable name string for the stepper
     * @param maxSpeed Maximum speed in steps per second (default: 1000)
     * @param acceleration Acceleration in steps per second per second (default: 500)
     */
    Stepper(int stepPin, int dirPin, const String &id, const String &name, 
            float maxSpeed = 1000.0, float acceleration = 500.0);

    /**
     * @brief Constructor for 4-pin stepper (HALF4WIRE - 28BYJ-48)
     * @param pin1 GPIO pin number for motor pin 1
     * @param pin2 GPIO pin number for motor pin 2
     * @param pin3 GPIO pin number for motor pin 3
     * @param pin4 GPIO pin number for motor pin 4
     * @param id Unique identifier string for the stepper
     * @param name Human-readable name string for the stepper
     * @param maxSpeed Maximum speed in steps per second (default: 500)
     * @param acceleration Acceleration in steps per second per second (default: 250)
     */
    Stepper(int pin1, int pin2, int pin3, int pin4, const String &id, const String &name,
            float maxSpeed = 500.0, float acceleration = 250.0);

    /**
     * @brief Setup function to initialize the stepper motor
     * Must be called in setup() before using the stepper
     */
    void setup();

    // Device interface implementation
    String getId() const override { return _id; }
    String getName() const override { return _name; }
    String getType() const override { return _type; }
    void loop() override; // Handle stepper motor movement

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;

    // Stepper-specific operations
    /**
     * @brief Move the stepper motor by a specified number of steps
     * @param steps Number of steps to move (positive = forward, negative = backward)
     */
    void move(long steps);

    /**
     * @brief Move the stepper motor to an absolute position
     * @param position Absolute position to move to
     */
    void moveTo(long position);

    /**
     * @brief Set the maximum speed of the stepper motor
     * @param speed Maximum speed in steps per second
     */
    void setMaxSpeed(float speed);

    /**
     * @brief Set the acceleration of the stepper motor
     * @param acceleration Acceleration in steps per second per second
     */
    void setAcceleration(float acceleration);

    /**
     * @brief Get current position of the stepper motor
     * @return Current position in steps
     */
    long getCurrentPosition() const;

    /**
     * @brief Get target position of the stepper motor
     * @return Target position in steps
     */
    long getTargetPosition() const;

    /**
     * @brief Check if the stepper motor is currently moving
     * @return true if moving, false if stopped
     */
    bool isRunning() const;

    /**
     * @brief Stop the stepper motor immediately
     */
    void stop();

private:
    AccelStepper _stepper;       ///< AccelStepper library instance
    String _id;                  ///< Unique identifier string for the stepper
    String _name;                ///< Human-readable name string for the stepper
    String _type = "STEPPER";    ///< Type of the device
    float _maxSpeed;             ///< Maximum speed in steps per second
    float _acceleration;         ///< Acceleration in steps per second per second
    bool _isMoving = false;      ///< Current movement state
    
    // Pin configuration
    bool _is4Pin = false;        ///< True if 4-pin configuration, false if 2-pin
    int _pin1, _pin2, _pin3, _pin4; ///< Pin numbers (all used for 4-pin, only pin1&pin2 for 2-pin)
    String _stepperType;         ///< Type string for debugging ("DRIVER" or "HALF4WIRE")
};

#endif // STEPPER_H
