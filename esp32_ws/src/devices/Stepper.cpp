/**
 * @file Stepper.cpp
 * @brief Implementation of Stepper motor control class
 *
 * This file contains the implementation of the Stepper class methods
 * for controlling stepper motors with acceleration and precise positioning
 * in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Stepper.h"

/**
 * @brief Constructor for 2-pin stepper (DRIVER - NEMA 17 with driver)
 *
 * Initializes the Stepper object for use with stepper motor drivers (like A4988, DRV8825)
 * that require step and direction pins.
 * @param stepPin GPIO pin number for the step signal
 * @param dirPin GPIO pin number for the direction signal
 * @param id Unique identifier string for the stepper
 * @param name Human-readable name string for the stepper
 * @param maxSpeed Maximum speed in steps per second
 * @param acceleration Acceleration in steps per second per second
 */
Stepper::Stepper(int stepPin, int dirPin, const String &id, const String &name, 
                 float maxSpeed, float acceleration)
    : _stepper(AccelStepper::DRIVER, stepPin, dirPin),
      _id(id), _name(name), _maxSpeed(maxSpeed), _acceleration(acceleration),
      _is4Pin(false), _pin1(stepPin), _pin2(dirPin), _pin3(-1), _pin4(-1),
      _stepperType("DRIVER")
{
    Serial.println("Stepper [" + _id + "]: Created DRIVER type on pins " + String(_pin1) + " (step), " + String(_pin2) + " (dir)");
}

/**
 * @brief Constructor for 4-pin stepper (HALF4WIRE - 28BYJ-48)
 *
 * Initializes the Stepper object for direct connection to 4-wire stepper motors
 * like the 28BYJ-48.
 * @param pin1 GPIO pin number for motor pin 1
 * @param pin2 GPIO pin number for motor pin 2
 * @param pin3 GPIO pin number for motor pin 3
 * @param pin4 GPIO pin number for motor pin 4
 * @param id Unique identifier string for the stepper
 * @param name Human-readable name string for the stepper
 * @param maxSpeed Maximum speed in steps per second
 * @param acceleration Acceleration in steps per second per second
 */
Stepper::Stepper(int pin1, int pin2, int pin3, int pin4, const String &id, const String &name,
                 float maxSpeed, float acceleration)
    : _stepper(AccelStepper::HALF4WIRE, pin1, pin3, pin2, pin4),
      _id(id), _name(name), _maxSpeed(maxSpeed), _acceleration(acceleration),
      _is4Pin(true), _pin1(pin1), _pin2(pin2), _pin3(pin3), _pin4(pin4),
      _stepperType("HALF4WIRE")
{
    Serial.println("Stepper [" + _id + "]: Created HALF4WIRE type on pins " + String(_pin1) + 
                   ", " + String(_pin2) + ", " + String(_pin3) + ", " + String(_pin4));
}

/**
 * @brief Setup function to initialize the stepper motor
 * Must be called in setup() before using the stepper
 */
void Stepper::setup()
{
    // Set maximum speed and acceleration
    _stepper.setMaxSpeed(_maxSpeed);
    _stepper.setAcceleration(_acceleration);
    
    // Set current position to 0
    _stepper.setCurrentPosition(0);
    
    if (_is4Pin)
    {
        Serial.println("Stepper [" + _id + "]: Setup complete (HALF4WIRE) on pins " + 
                       String(_pin1) + ", " + String(_pin2) + ", " + String(_pin3) + ", " + String(_pin4));
    }
    else
    {
        Serial.println("Stepper [" + _id + "]: Setup complete (DRIVER) on pins " + 
                       String(_pin1) + " (step), " + String(_pin2) + " (dir)");
    }
    
    Serial.println("Stepper [" + _id + "]: Max speed: " + String(_maxSpeed) + 
                   " steps/s, Acceleration: " + String(_acceleration) + " steps/s²");
}

/**
 * @brief Loop function for continuous stepper motor operations
 *
 * Handles stepper motor movement using AccelStepper's run() method.
 * Should be called repeatedly in the main loop for smooth motion.
 */
void Stepper::loop()
{
    bool wasMoving = _isMoving;
    _isMoving = _stepper.run();
    
    // If stepper just stopped moving, notify state change
    if (wasMoving && !_isMoving)
    {
        // Serial.println("Stepper [" + _id + "]: Movement completed at position " + String(_stepper.currentPosition()));
        // notifyStateChange();
    }
}

/**
 * @brief Move the stepper motor by a specified number of steps
 * @param steps Number of steps to move (positive = forward, negative = backward)
 */
void Stepper::move(long steps)
{
    //Serial.println("Stepper [" + _id + "]: Moving " + String(steps) + " steps");
    _stepper.move(steps);
    _isMoving = true;
    // notifyStateChange();
}

/**
 * @brief Move the stepper motor to an absolute position
 * @param position Absolute position to move to
 */
void Stepper::moveTo(long position)
{
    Serial.println("Stepper [" + _id + "]: Moving to position " + String(position));
    _stepper.moveTo(position);
    _isMoving = true;
    // notifyStateChange();
}

/**
 * @brief Set the maximum speed of the stepper motor
 * @param speed Maximum speed in steps per second
 */
void Stepper::setMaxSpeed(float speed)
{
    _maxSpeed = speed;
    _stepper.setMaxSpeed(speed);
    Serial.println("Stepper [" + _id + "]: Max speed set to " + String(speed) + " steps/s");
}

/**
 * @brief Set the acceleration of the stepper motor
 * @param acceleration Acceleration in steps per second per second
 */
void Stepper::setAcceleration(float acceleration)
{
    _acceleration = acceleration;
    _stepper.setAcceleration(acceleration);
    Serial.println("Stepper [" + _id + "]: Acceleration set to " + String(acceleration) + " steps/s²");
}

/**
 * @brief Get current position of the stepper motor
 * @return Current position in steps
 */
long Stepper::getCurrentPosition() const
{
    return const_cast<AccelStepper&>(_stepper).currentPosition();
}

/**
 * @brief Get target position of the stepper motor
 * @return Target position in steps
 */
long Stepper::getTargetPosition() const
{
    return const_cast<AccelStepper&>(_stepper).targetPosition();
}

/**
 * @brief Check if the stepper motor is currently moving
 * @return true if moving, false if stopped
 */
bool Stepper::isRunning() const
{
    return _isMoving;
}

/**
 * @brief Stop the stepper motor immediately
 */
void Stepper::stop()
{
    Serial.println("Stepper [" + _id + "]: Emergency stop");
    _stepper.stop();
    _isMoving = false;
    // notifyStateChange();
}

/**
 * @brief Dynamic control function for stepper motor operations
 * @param action The action to perform (e.g., "move", "moveTo", "stop", "setSpeed", "setAcceleration")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Stepper::control(const String &action, JsonObject *payload)
{
    if (action == "move")
    {
        if (!payload || !(*payload)["steps"].is<long>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'move' payload - need steps");
            return false;
        }

        long steps = (*payload)["steps"].as<long>();
        move(steps);
        return true;
    }
    else if (action == "moveTo")
    {
        if (!payload || !(*payload)["position"].is<long>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'moveTo' payload - need position");
            return false;
        }

        long position = (*payload)["position"].as<long>();
        moveTo(position);
        return true;
    }
    else if (action == "stop")
    {
        stop();
        return true;
    }
    else if (action == "setSpeed")
    {
        if (!payload || !(*payload)["speed"].is<float>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'setSpeed' payload - need speed");
            return false;
        }

        float speed = (*payload)["speed"].as<float>();
        if (speed <= 0 || speed > 10000)
        {
            Serial.println("Stepper [" + _id + "]: Invalid speed " + String(speed) + " (range: 1-10000)");
            return false;
        }

        setMaxSpeed(speed);
        return true;
    }
    else if (action == "setAcceleration")
    {
        if (!payload || !(*payload)["acceleration"].is<float>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'setAcceleration' payload - need acceleration");
            return false;
        }

        float acceleration = (*payload)["acceleration"].as<float>();
        if (acceleration <= 0 || acceleration > 10000)
        {
            Serial.println("Stepper [" + _id + "]: Invalid acceleration " + String(acceleration) + " (range: 1-10000)");
            return false;
        }

        setAcceleration(acceleration);
        return true;
    }
    else
    {
        Serial.println("Stepper [" + _id + "]: Unknown action: " + action);
        return false;
    }
}

/**
 * @brief Get current state of the stepper motor
 * @return String containing JSON representation of the current state
 */
String Stepper::getState()
{
    JsonDocument doc;
    doc["type"] = _type;
    doc["name"] = _name;

    doc["currentPosition"] = getCurrentPosition();
    doc["targetPosition"] = getTargetPosition();
    doc["isRunning"] = isRunning();
    doc["maxSpeed"] = _maxSpeed;
    doc["acceleration"] = _acceleration;
    doc["stepperType"] = _stepperType;
    doc["is4Pin"] = _is4Pin;
    
    

    String result;
    serializeJson(doc, result);
    return result;
}
