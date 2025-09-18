
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
#include "Logging.h"

/**
 * @brief Constructor for Stepper motor
 *
 * Initializes the Stepper object with just ID and name.
 * Configuration must be done separately using configure2Pin() or configure4Pin()
 * @param id Unique identifier string for the stepper
 * @param name Human-readable name string for the stepper
 */
Stepper::Stepper(const String &id, const String &name)
    : Device(id, "stepper")
{

}

/**
 * @brief Destructor - cleans up AccelStepper instance
 */
Stepper::~Stepper()
{
    cleanupAccelStepper();
}

/**
 * @brief Setup function to initialize the stepper motor
 * Must be called in setup() before using the stepper
 */
void Stepper::setup()
{
    if (!_configured)
    {
        // MLOG_WARN("Stepper [%s]: Not configured yet - call configure2Pin() or configure4Pin() first", _id.c_str());
        return;
    }

    if (!_stepper)
    {
        MLOG_ERROR("Stepper [%s]: AccelStepper instance not initialized", _id.c_str());
        return;
    }

    // Set maximum speed and acceleration
    _stepper->setMaxSpeed(_maxSpeed);
    _stepper->setAcceleration(_maxAcceleration);

    // Set current position to 0
    _stepper->setCurrentPosition(0);
}

/**
 * @brief Loop function for continuous stepper motor operations
 *
 * Handles stepper motor movement using AccelStepper's run() method.
 * Should be called repeatedly in the main loop for smooth motion.
 */
void Stepper::loop()
{
    if (!_configured || !_stepper)
    {
        return;
    }

    bool wasMoving = _isMoving;
    _isMoving = _stepper->run();

    // If stepper just stopped moving, notify state change
    if (wasMoving && !_isMoving)
    {
        // Serial.println("Stepper [" + _id + "]: Movement completed at position " + String(_stepper->currentPosition()));
        notifyStateChange();
    }
}

/**
 * @brief Move the stepper motor by a specified number of steps
 * @param steps Number of steps to move (positive = forward, negative = backward)
 */
void Stepper::move(long steps)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot move", _id.c_str());
        return;
    }

    // Serial.println("Stepper [" + _id + "]: Moving " + String(steps) + " steps");
    _stepper->move(steps);
    _isMoving = true;
    notifyStateChange();
}

/**
 * @brief Move the stepper motor to an absolute position
 * @param position Absolute position to move to
 */
void Stepper::moveTo(long position)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot move to position", _id.c_str());
        return;
    }

    MLOG_INFO("Stepper [%s]: Moving to position %ld", _id.c_str(), position);
    _stepper->moveTo(position);
    _isMoving = true;
    // notifyStateChange();
}

/**
 * @brief Reset the stepper motor's current position to zero
 */
void Stepper::setCurrentPosition(long position)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot set current position", _id.c_str());
        return;
    }

    MLOG_INFO("Stepper [%s]: Resetting current position to %ld", _id.c_str(), position);
    _stepper->setCurrentPosition(position);
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
    if (_stepper)
    {
        _stepper->setMaxSpeed(maxSpeed);
    }
    MLOG_INFO("Stepper [%s]: Max speed set to %.2f steps/s", _id.c_str(), maxSpeed);
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
    if (_stepper)
    {
        _stepper->setAcceleration(maxAcceleration);
    }
    MLOG_INFO("Stepper [%s]: Acceleration set to %.2f steps/s^2", _id.c_str(), maxAcceleration);
}

/**
 * @brief Get current position of the stepper motor
 * @return Current position in steps
 */
long Stepper::getCurrentPosition() const
{
    if (!_configured || !_stepper)
    {
        return 0;
    }
    return _stepper->currentPosition();
}

/**
 * @brief Get target position of the stepper motor
 * @return Target position in steps
 */
long Stepper::getTargetPosition() const
{
    if (!_configured || !_stepper)
    {
        return 0;
    }
    return _stepper->targetPosition();
}

/**
 * @brief Check if the stepper motor is currently moving
 * @return true if moving, false if stopped
 */
bool Stepper::isMoving() const
{
    return _isMoving;
}

/**
 * @brief Stop the stepper motor immediately
 */
void Stepper::stop()
{
    MLOG_WARN("Stepper [%s]: Emergency stop", _id.c_str());
    if (_stepper)
    {
        _stepper->stop();
    }
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
        MLOG_INFO("Stepper [%s]: Move %ld steps (Speed: %.2f, Acceleration: %.2f)", _id.c_str(), steps, _maxSpeed, _maxAcceleration);
        move(steps);
        return true;
    }

    /*
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
        if (!payload || !(*payload)["maxAcceleration"].is<float>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'setAcceleration' payload - need maxAcceleration");
            return false;
        }

        float maxAcceleration = (*payload)["maxAcceleration"].as<float>();
        if (maxAcceleration <= 0 || maxAcceleration > 10000)
        {
            Serial.println("Stepper [" + _id + "]: Invalid acceleration " + String(acceleration) + " (range: 1-10000)");
            return false;
        }

        setAcceleration(maxAcceleration);
        return true;
    }
    */
    else
    {
        MLOG_WARN("Stepper [%s]: Unknown action: '%s'", _id.c_str(), action.c_str());
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
    doc["isMoving"] = isMoving();
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
    if (_configured)
    {
        pins.push_back(_pin1);
        pins.push_back(_pin2);
        if (_is4Pin)
        {
            pins.push_back(_pin3);
            pins.push_back(_pin4);
        }
    }
    return pins;
}

/**
 * @brief Configure stepper for 2-pin mode (DRIVER - NEMA 17 with driver)
 * @param stepPin GPIO pin number for the step signal
 * @param dirPin GPIO pin number for the direction signal
 * @param maxSpeed Maximum speed in steps per second
 * @param acceleration Acceleration in steps per second per second
 */
void Stepper::configure2Pin(int stepPin, int dirPin, float maxSpeed, float acceleration)
{
    cleanupAccelStepper();

    _is4Pin = false;
    _pin1 = stepPin;
    _pin2 = dirPin;
    _pin3 = -1;
    _pin4 = -1;
    _maxSpeed = maxSpeed;
    _maxAcceleration = acceleration;
    _stepperType = "DRIVER";

    initializeAccelStepper();
    _configured = true;

    MLOG_INFO("Stepper [%s]: Configured as DRIVER type on pins %d (step), %d (dir)", _id.c_str(), _pin1, _pin2);
}

/**
 * @brief Configure stepper for 4-pin mode (HALF4WIRE - 28BYJ-48)
 * @param pin1 GPIO pin number for motor pin 1
 * @param pin2 GPIO pin number for motor pin 2
 * @param pin3 GPIO pin number for motor pin 3
 * @param pin4 GPIO pin number for motor pin 4
 * @param maxSpeed Maximum speed in steps per second
 * @param acceleration Acceleration in steps per second per second
 */
void Stepper::configure4Pin(int pin1, int pin2, int pin3, int pin4, float maxSpeed, float acceleration)
{
    cleanupAccelStepper();

    _is4Pin = true;
    _pin1 = pin1;
    _pin2 = pin2;
    _pin3 = pin3;
    _pin4 = pin4;
    _maxSpeed = maxSpeed;
    _maxAcceleration = acceleration;
    _stepperType = "HALF4WIRE";

    initializeAccelStepper();
    _configured = true;

    MLOG_INFO("Stepper [%s]: Configured as HALF4WIRE type on pins %d, %d, %d, %d", _id.c_str(), _pin1, _pin2, _pin3, _pin4);
}

/**
 * @brief Initialize the AccelStepper instance based on current configuration
 */
void Stepper::initializeAccelStepper()
{
    if (_stepper)
    {
        delete _stepper;
        _stepper = nullptr;
    }

    if (_is4Pin)
    {
        _stepper = new AccelStepper(AccelStepper::HALF4WIRE, _pin1, _pin3, _pin2, _pin4);
    }
    else
    {
        _stepper = new AccelStepper(AccelStepper::DRIVER, _pin1, _pin2);
    }
}

/**
 * @brief Clean up existing AccelStepper instance
 */
void Stepper::cleanupAccelStepper()
{
    if (_stepper)
    {
        delete _stepper;
        _stepper = nullptr;
    }
}

/**
 * @brief Get configuration as JSON
 * @return JsonObject containing the current configuration
 */
String Stepper::getConfig() const
{
    JsonDocument doc;
    JsonObject config = doc.to<JsonObject>();

    config["configured"] = _configured;
    if (_configured)
    {
        config["stepperType"] = _stepperType;
        config["is4Pin"] = _is4Pin;
        config["maxSpeed"] = _maxSpeed;
        config["acceleration"] = _maxAcceleration;

        if (_is4Pin)
        {
            JsonArray pins = config["pins"].to<JsonArray>();
            pins.add(_pin1);
            pins.add(_pin2);
            pins.add(_pin3);
            pins.add(_pin4);
        }
        else
        {
            JsonObject pinConfig = config["pins"].to<JsonObject>();
            pinConfig["stepPin"] = _pin1;
            pinConfig["dirPin"] = _pin2;
        }
    }

    String result;
    serializeJson(config, result);
    return result;
}

/**
 * @brief Set configuration from JSON
 * @param config Pointer to JSON object containing configuration
 */
void Stepper::setConfig(JsonObject *config)
{
    if (!config)
    {
        MLOG_WARN("Stepper [%s]: Null config provided", _id.c_str());
        return;
    }

    // Check if we should configure as 2-pin or 4-pin
    if ((*config)["stepperType"].is<String>())
    {
        String stepperType = (*config)["stepperType"].as<String>();

        if (stepperType == "DRIVER")
        {
            // Configure as 2-pin
            if ((*config)["pins"]["stepPin"].is<int>() && (*config)["pins"]["dirPin"].is<int>())
            {
                int stepPin = (*config)["pins"]["stepPin"].as<int>();
                int dirPin = (*config)["pins"]["dirPin"].as<int>();
                float maxSpeed = (*config)["maxSpeed"].is<float>() ? (*config)["maxSpeed"].as<float>() : 1000.0;
                float acceleration = (*config)["acceleration"].is<float>() ? (*config)["acceleration"].as<float>() : 500.0;

                configure2Pin(stepPin, dirPin, maxSpeed, acceleration);
                MLOG_INFO("Stepper [%s]: Configured from JSON as DRIVER", _id.c_str());
            }
            else
            {
                MLOG_WARN("Stepper [%s]: Invalid 2-pin configuration in JSON", _id.c_str());
            }
        }
        else if (stepperType == "HALF4WIRE")
        {
            // Configure as 4-pin
            if ((*config)["pins"].is<JsonArray>() && (*config)["pins"].size() == 4)
            {
                JsonArray pins = (*config)["pins"].as<JsonArray>();
                int pin1 = pins[0].as<int>();
                int pin2 = pins[1].as<int>();
                int pin3 = pins[2].as<int>();
                int pin4 = pins[3].as<int>();
                float maxSpeed = (*config)["maxSpeed"].is<float>() ? (*config)["maxSpeed"].as<float>() : 500.0;
                float acceleration = (*config)["acceleration"].is<float>() ? (*config)["acceleration"].as<float>() : 250.0;

                configure4Pin(pin1, pin2, pin3, pin4, maxSpeed, acceleration);
                MLOG_INFO("Stepper [%s]: Configured from JSON as HALF4WIRE", _id.c_str());
            }
            else
            {
                MLOG_WARN("Stepper [%s]: Invalid 4-pin configuration in JSON", _id.c_str());
            }
        }
        else
        {
            MLOG_WARN("Stepper [%s]: Unknown stepperType '%s'", _id.c_str(), stepperType.c_str());
        }
    }
    else
    {
        MLOG_WARN("Stepper [%s]: No stepperType specified in config", _id.c_str());
    }
}
