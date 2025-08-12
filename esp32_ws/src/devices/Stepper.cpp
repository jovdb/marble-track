
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
    : Device(id, name, "STEPPER"), _stepper(AccelStepper::DRIVER, stepPin, dirPin),
      _maxSpeed(maxSpeed), _maxAcceleration(acceleration),
      _is4Pin(false), _pin1(stepPin), _pin2(dirPin), _pin3(-1), _pin4(-1),
      _stepperType("DRIVER")
{
    log("Created DRIVER type on pins %d (step), %d (dir)\n", _pin1, _pin2);
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
    : Device(id, name, "STEPPER"), _stepper(AccelStepper::HALF4WIRE, pin1, pin3, pin2, pin4),
      _maxSpeed(maxSpeed), _maxAcceleration(acceleration),
      _is4Pin(true), _pin1(pin1), _pin2(pin2), _pin3(pin3), _pin4(pin4),
      _stepperType("HALF4WIRE")
{
    log("Created HALF4WIRE type on pins %d, %d, %d, %d\n", _pin1, _pin2, _pin3, _pin4);
}

/**
 * @brief Setup function to initialize the stepper motor
 * Must be called in setup() before using the stepper
 */
void Stepper::setup()
{
    // Set maximum speed and acceleration
    _stepper.setMaxSpeed(_maxSpeed);
    _stepper.setAcceleration(_maxAcceleration);

    // Set current position to 0
    _stepper.setCurrentPosition(0);

    if (_is4Pin)
    {
    log("Setup complete (HALF4WIRE) on pins %d, %d, %d, %d\n", _pin1, _pin2, _pin3, _pin4);
    }
    else
    {
    log("Setup complete (DRIVER) on pins %d (step), %d (dir)\n", _pin1, _pin2);
    }

    log("Max speed: %.2f steps/s, Acceleration: %.2f steps/s²\n", _maxSpeed, _maxAcceleration);
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
    // Serial.println("Stepper [" + _id + "]: Moving " + String(steps) + " steps");
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
    log("Moving to position %ld\n", position);
    _stepper.moveTo(position);
    _isMoving = true;
    // notifyStateChange();
}

/**
 * @brief Reset the stepper motor's current position to zero
 */
void Stepper::setCurrentPosition(long position)
{
    log("Resetting current position to 0\n");
    _stepper.setCurrentPosition(0);
    // Optionally, update state or notify if needed
    // notifyStateChange();
}
/**
 * @brief Set the maximum speed of the stepper motor
 * @param maxSpeed Maximum speed in steps per second
 */
void Stepper::setMaxSpeed(float maxSpeed)
{
    if (_maxSpeed == maxSpeed)
        return;

    _maxSpeed = maxSpeed;
    _stepper.setMaxSpeed(maxSpeed);
    log("Max speed set to %.2f steps/s\n", maxSpeed);
}

/**
 * @brief Set the acceleration of the stepper motor
 * @param maxAcceleration Acceleration in steps per second per second
 */
void Stepper::setAcceleration(float maxAcceleration)
{
    if (_maxAcceleration == maxAcceleration)
        return;
    _maxAcceleration = maxAcceleration;
    _stepper.setAcceleration(maxAcceleration);
    log("Acceleration set to %.2f steps/s²\n", maxAcceleration);
}

/**
 * @brief Get current position of the stepper motor
 * @return Current position in steps
 */
long Stepper::getCurrentPosition() const
{
    return const_cast<AccelStepper &>(_stepper).currentPosition();
}

/**
 * @brief Get target position of the stepper motor
 * @return Target position in steps
 */
long Stepper::getTargetPosition() const
{
    return const_cast<AccelStepper &>(_stepper).targetPosition();
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
    log("Emergency stop\n");
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

        if (payload && (*payload)["maxAcceleration"].is<long>())
        {
            long maxAcceleration = (*payload)["maxAcceleration"].as<long>();
            setAcceleration(maxAcceleration);
        }

        if (payload && (*payload)["maxSpeed"].is<long>())
        {
            long maxSpeed = (*payload)["maxSpeed"].as<long>();
            setMaxSpeed(maxSpeed);
        }

        if (!payload || !(*payload)["steps"].is<long>())
        {
            return false;
        }

        long steps = (*payload)["steps"].as<long>();
    log("Move %ld steps (Speed: %.2f, Acceleration: %.2f)\n", steps, _maxSpeed, _maxAcceleration);
        move(steps);
        return true;
    }

    /*
    else if (action == "moveTo")
    {
        if (!payload || !(*payload)["position"].is<long>())
        {
            log("Invalid 'moveTo' payload - need position\n");
            return false;
        }

        long position = (*payload)["position"].as<long>();
        moveTo(position);
        return true;
    }
        */
    else if (action == "stop")
    {
        stop();
        return true;
    }
    /*
    else if (action == "setSpeed")
    {
        if (!payload || !(*payload)["speed"].is<float>())
        {
            log("Invalid 'setSpeed' payload - need speed\n");
            return false;
        }

        float speed = (*payload)["speed"].as<float>();
        if (speed <= 0 || speed > 10000)
        {
            log("Invalid speed %.2f (range: 1-10000)\n", speed);
            return false;
        }

        setMaxSpeed(speed);
        return true;
    }
    else if (action == "setAcceleration")
    {
        if (!payload || !(*payload)["maxAcceleration"].is<float>())
        {
            log("Invalid 'setAcceleration' payload - need maxAcceleration\n");
            return false;
        }

        float maxAcceleration = (*payload)["maxAcceleration"].as<float>();
        if (maxAcceleration <= 0 || maxAcceleration > 10000)
        {
            log("Invalid acceleration %.2f (range: 1-10000)\n", acceleration);
            return false;
        }

        setAcceleration(maxAcceleration);
        return true;
    }
    */
    else
    {
    log("Unknown action: '%s'\n", action.c_str());
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
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }

    doc["currentPosition"] = getCurrentPosition();
    doc["targetPosition"] = getTargetPosition();
    doc["isRunning"] = isRunning();
    doc["maxSpeed"] = _maxSpeed;
    doc["acceleration"] = _maxAcceleration;
    doc["stepperType"] = _stepperType;
    doc["is4Pin"] = _is4Pin;

    String result;
    serializeJson(doc, result);
    return result;
}

std::vector<int> Stepper::getPins() const
{
    std::vector<int> pins;
    pins.push_back(_pin1);
    pins.push_back(_pin2);
    if (_is4Pin)
    {
        pins.push_back(_pin3);
        pins.push_back(_pin4);
    }
    return pins;
}
