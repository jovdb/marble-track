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

/**
 * @class ServoDevice
 * @brief Servo motor control class
 *
 * Provides servo motor control functionality with angle management and
 * automatic initialization. Supports angle range 0-180 degrees.
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
     */
    ServoDevice(int pin, const String &id, const String &name, int initialAngle = 90);

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
    String getId() const override { return id; }
    String getName() const override { return name; }
    String getType() const override { return type; }
    void loop() override {} // No periodic operations needed

    // Controllable functionality
    bool isControllable() const override { return true; }
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;

    // Servo-specific operations
    void setAngle(int angle);
    int getAngle() const { return currentAngle; }

private:
    int pin;                    ///< GPIO pin number for the servo
    String id;                  ///< Unique identifier string for the servo
    String name;                ///< Human-readable name string for the servo
    String type = "SERVO";      ///< Type of the device
    int currentAngle;           ///< Current angle of the servo (0-180)

    /**
     * @brief Validate and constrain angle to valid range
     * @param angle Input angle to validate
     * @return Constrained angle (0-180)
     */
    int constrainAngle(int angle);
};

#endif // SERVO_H
