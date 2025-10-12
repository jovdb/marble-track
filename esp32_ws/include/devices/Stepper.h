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
 * Supports both 2-pin (DRIVER for NEMA 17) and 4-pin (HALF4WIRE / FULL4WIRE) configurations.
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
     */
    Stepper(const String &id);

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
     * @param speed Optional maximum speed in steps per second (uses configured speed if not provided)
     * @param acceleration Optional acceleration in steps per second per second (uses configured acceleration if not provided)
     */
    void move(long steps, float speed = -1, float acceleration = -1);

    /**
     * @brief Move the stepper motor to an absolute position
     * @param position Absolute position to move to
     * @param speed Optional maximum speed in steps per second (uses configured speed if not provided)
     * @param acceleration Optional acceleration in steps per second per second (uses configured acceleration if not provided)
     */
    void moveTo(long position, float speed = -1, float acceleration = -1);

    /**
     * @brief Set the maximum speed of the stepper motor
     * @param maxSpeed Maximum speed in steps per second
     */
    void setMaxSpeed(float maxSpeed);

    /**
     * @brief Set the acceleration of the stepper motor
     * @param maxAcceleration Acceleration in steps per second per second
     */
    void setMaxAcceleration(float maxAcceleration);

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
     * @param acceleration Optional deceleration rate in steps per second per second (uses configured acceleration if not provided)
     */
    void stop(float acceleration = -1);

    // Configuration setters/getters
    /**
     * @brief Configure stepper for 2-pin mode (DRIVER - NEMA 17 with driver)
     * @param stepPin GPIO pin number for the step signal
     * @param dirPin GPIO pin number for the direction signal
     * @param enablePin GPIO pin number for the enable signal (-1 to disable)
     */
    void configure2Pin(int stepPin, int dirPin, int enablePin = -1);

    /**
     * @brief Configure stepper for 4-pin mode (HALF4WIRE or FULL4WIRE)
     * @param pin1 GPIO pin number for motor pin 1
     * @param pin2 GPIO pin number for motor pin 2
     * @param pin3 GPIO pin number for motor pin 3
     * @param pin4 GPIO pin number for motor pin 4
     * @param enablePin GPIO pin number for the enable signal (-1 to disable)
     */
    void configure4Pin(int pin1, int pin2, int pin3, int pin4,
                       AccelStepper::MotorInterfaceType mode = AccelStepper::HALF4WIRE,
                       const char *typeLabel = "HALF4WIRE",
                       int enablePin = -1);

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
    * @return Type string ("DRIVER", "HALF4WIRE", or "FULL4WIRE")
     */
    String getStepperType() const { return _stepperType; }

    /**
     * @brief Check if stepper is configured
     * @return true if configured, false otherwise
     */
    bool isConfigured() const { return _configured; }

    /**
     * @brief Get enable pin number
     * @return Enable pin number (-1 if not configured)
     */
    int getEnablePin() const { return _enablePin; }

private:
    AccelStepper *_stepper = nullptr; ///< AccelStepper library instance
    float _maxSpeed = 1000.0;         ///< Maximum speed in steps per second
    float _maxAcceleration = 500.0;   ///< Acceleration in steps per second per second
    bool _isMoving = false;           ///< Current movement state
    bool _configured = false;         ///< True if stepper has been configured

    // Pin configuration
    bool _is4Pin = false;                               ///< True if 4-pin configuration, false if 2-pin
    int _pin1 = -1, _pin2 = -1, _pin3 = -1, _pin4 = -1; ///< Pin numbers (all used for 4-pin, only pin1&pin2 for 2-pin)
    int _enablePin = -1;                                ///< Enable pin number (-1 if not used)
    String _stepperType = "";                           ///< Type string for reporting ("DRIVER", "HALF4WIRE", "FULL4WIRE")
    AccelStepper::MotorInterfaceType _interfaceType = AccelStepper::DRIVER; ///< Current motor interface

    /**
     * @brief Initialize the AccelStepper instance based on current configuration
     */
    void initializeAccelStepper();

    /**
     * @brief Clean up existing AccelStepper instance
     */
    void cleanupAccelStepper();

    /**
     * @brief Enable the stepper motor (set enable pin high if configured)
     */
    void enableStepper();

    /**
     * @brief Disable the stepper motor (set enable pin low if configured)
     */
    void disableStepper();
};

#endif // STEPPER_H
