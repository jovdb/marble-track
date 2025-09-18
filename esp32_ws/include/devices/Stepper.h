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
     * @brief Reset the stepper motor's current position to zero
     */
    void setCurrentPosition(long position);
    /**
     * @brief Get all pin numbers used by this stepper
     * @return std::vector<int> of pin numbers
     */
    std::vector<int> getPins() const override;
    /**
     * @brief Constructor for Stepper motor
     * @param id Unique identifier string for the stepper
     * @param name Human-readable name string for the stepper
     */
    Stepper(const String &id, const String &name);
    
    /**
     * @brief Destructor - cleans up AccelStepper instance
     */
    ~Stepper();

    /**
     * @brief Setup function to initialize the stepper motor
     * Must be called in setup() before using the stepper
     */
    void setup();

    void loop() override; // Handle stepper motor movement

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;

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
     * @param maxSpeed Maximum speed in steps per second
     */
    void setMaxSpeed(float maxSpeed);

    /**
     * @brief Set the acceleration of the stepper motor
     * @param maxAcceleration Acceleration in steps per second per second
     */
    void setAcceleration(float maxAcceleration);

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
    bool isMoving() const;

    /**
     * @brief Stop the stepper motor immediately
     */
    void stop();

    // Configuration setters/getters
    /**
     * @brief Configure stepper for 2-pin mode (DRIVER - NEMA 17 with driver)
     * @param stepPin GPIO pin number for the step signal
     * @param dirPin GPIO pin number for the direction signal
     * @param maxSpeed Maximum speed in steps per second (default: 1000)
     * @param acceleration Acceleration in steps per second per second (default: 500)
     */
    void configure2Pin(int stepPin, int dirPin, float maxSpeed = 1000.0, float acceleration = 500.0);

    /**
     * @brief Configure stepper for 4-pin mode (HALF4WIRE - 28BYJ-48)
     * @param pin1 GPIO pin number for motor pin 1
     * @param pin2 GPIO pin number for motor pin 2
     * @param pin3 GPIO pin number for motor pin 3
     * @param pin4 GPIO pin number for motor pin 4
     * @param maxSpeed Maximum speed in steps per second (default: 500)
     * @param acceleration Acceleration in steps per second per second (default: 250)
     */
    void configure4Pin(int pin1, int pin2, int pin3, int pin4, float maxSpeed = 500.0, float acceleration = 250.0);

    /**
     * @brief Get maximum speed
     * @return Maximum speed in steps per second
     */
    float getMaxSpeed() const { return _maxSpeed; }

    /**
     * @brief Get acceleration
     * @return Acceleration in steps per second per second
     */
    float getAcceleration() const { return _maxAcceleration; }

    /**
     * @brief Get stepper type
     * @return Type string ("DRIVER" or "HALF4WIRE")
     */
    String getStepperType() const { return _stepperType; }

    /**
     * @brief Check if stepper is configured
     * @return true if configured, false otherwise
     */
    bool isConfigured() const { return _configured; }

private:
    AccelStepper *_stepper = nullptr; ///< AccelStepper library instance
    float _maxSpeed = 1000.0;         ///< Maximum speed in steps per second
    float _maxAcceleration = 500.0;   ///< Acceleration in steps per second per second
    bool _isMoving = false;           ///< Current movement state
    bool _configured = false;         ///< True if stepper has been configured

    // Pin configuration
    bool _is4Pin = false;           ///< True if 4-pin configuration, false if 2-pin
    int _pin1 = -1, _pin2 = -1, _pin3 = -1, _pin4 = -1; ///< Pin numbers (all used for 4-pin, only pin1&pin2 for 2-pin)
    String _stepperType = "";       ///< Type string for debugging ("DRIVER" or "HALF4WIRE")

    /**
     * @brief Initialize the AccelStepper instance based on current configuration
     */
    void initializeAccelStepper();

    /**
     * @brief Clean up existing AccelStepper instance
     */
    void cleanupAccelStepper();
};

#endif // STEPPER_H
