
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
 * Initializes the Stepper object with just an identifier.
 * Configuration must be done separately using configure2Pin() or configure4Pin()
 * @param id Unique identifier string for the stepper
 */
Stepper::Stepper(const String &id)
    : Device(id, "stepper")
{
    _name = id;
    _maxSpeed = 1000;
    _maxAcceleration = 500;
    _interfaceType = AccelStepper::DRIVER;
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

    if (_stepper)
    {
        _stepper->setMaxSpeed(_maxSpeed);
        _stepper->setAcceleration(_maxAcceleration);
    }

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

    // If stepper just stopped moving, notify state change and disable stepper
    if (wasMoving && !_isMoving)
    {
        MLOG_INFO("Stepper [%s]: Movement completed at position %ld", _id.c_str(), _stepper->currentPosition());
        disableStepper(); // Disable stepper after movement completes
        notifyStateChange();
    }
}

/**
 * @brief Move the stepper motor by a specified number of steps
 * @param steps Number of steps to move (positive = forward, negative = backward)
 * @param speed Optional maximum speed in steps per second (uses configured speed if not provided)
 * @param acceleration Optional acceleration in steps per second per second (uses configured acceleration if not provided)
 * @return true if move was initiated, false if not configured
 */
bool Stepper::move(long steps, float speed, float acceleration)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot move", _id.c_str());
        return false;
    }

    if (speed <= 0 || speed > _maxSpeed)
    {
        speed = _maxSpeed;
    }
    if (acceleration <= 0 || acceleration > _maxAcceleration)
    {
        acceleration = _maxAcceleration;
    }

    MLOG_INFO("Stepper [%s]: Moving %ld steps (Speed: %.2f, Acceleration: %.2f)", _id.c_str(), steps, speed, acceleration);
    enableStepper(); // Enable stepper before movement
    _stepper->setAcceleration(acceleration);
    _stepper->setMaxSpeed(speed);
    _stepper->move(steps);
    _isMoving = true;
    notifyStateChange();
    return true;
}

/**
 * @brief Move the stepper motor to an absolute position
 * @param position Absolute position to move to
 * @param speed Optional maximum speed in steps per second (uses configured speed if not provided)
 * @param acceleration Optional acceleration in steps per second per second (uses configured acceleration if not provided)
 * @return true if move was initiated, false if not configured
 */
bool Stepper::moveTo(long position, float speed, float acceleration)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot move to position", _id.c_str());
        return false;
    }

    if (speed <= 0 || speed > _maxSpeed)
    {
        speed = _maxSpeed;
    }
    if (acceleration <= 0 || acceleration > _maxAcceleration)
    {
        acceleration = _maxAcceleration;
    }

    MLOG_INFO("Stepper [%s]: Moving to position %ld (Speed: %.2f, Acceleration: %.2f)", _id.c_str(), position, speed, acceleration);
    enableStepper(); // Enable stepper before movement
    _stepper->setMaxSpeed(speed);
    _stepper->setAcceleration(acceleration);
    _stepper->moveTo(position);
    _isMoving = true;
    notifyStateChange();
    return true;
}

/**
 * @brief Stop the stepper motor immediately
 * @param acceleration Optional deceleration rate in steps per second per second (uses configured acceleration if not provided)
 * @return true if stop was initiated, false if not configured
 */
bool Stepper::stop(float acceleration)
{
    if (!_configured || !_stepper)
    {
        return false;
    }

    if (acceleration <= 0 || acceleration > _maxAcceleration)
    {
        acceleration = _maxAcceleration;
    }

    MLOG_WARN("Stepper [%s]: Stop (Deceleration: %.2f)", _id.c_str(), acceleration);
    _stepper->setAcceleration(acceleration);
    _stepper->stop();
    disableStepper(); // Disable stepper immediately when stopped
    _isMoving = false;
    notifyStateChange();
    return true;
}

/**
 * @brief Reset the stepper motor's current position to zero
 * @return true if position was set, false if not configured
 */
bool Stepper::setCurrentPosition(long position)
{
    if (!_configured || !_stepper)
    {
        MLOG_WARN("Stepper [%s]: Not configured - cannot set current position", _id.c_str());
        return false;
    }

    MLOG_INFO("Stepper [%s]: Resetting current position to %ld", _id.c_str(), position);
    _stepper->setCurrentPosition(position);

    // notify the new values
    notifyStateChange();
    return true;
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
void Stepper::setMaxAcceleration(float maxAcceleration)
{
    if (_maxAcceleration == maxAcceleration)
        return;
    _maxAcceleration = maxAcceleration;
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
 * @brief Dynamic control function for stepper motor operations
 * @param action The action to perform (e.g., "move", "moveTo", "stop", "setSpeed", "setMaxAcceleration")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Stepper::control(const String &action, JsonObject *payload)
{
    if (action == "move")
    {
        if (!payload || !(*payload)["steps"].is<long>())
        {
            return false;
        }

        long steps = (*payload)["steps"].as<long>();

        float speed = -1;
        float acceleration = -1;
        if (payload && (*payload)["speed"].is<float>())
        {
            speed = (*payload)["speed"].as<float>();
        }

        if (payload && (*payload)["acceleration"].is<float>())
        {
            acceleration = (*payload)["acceleration"].as<float>();
        }

        MLOG_INFO("Stepper [%s]: Move %ld steps (Speed: %.2f, Acceleration: %.2f)", _id.c_str(), steps,
                  speed > 0 ? speed : _maxSpeed, acceleration > 0 ? acceleration : _maxAcceleration);
        return move(steps, speed, acceleration);
    }

    else if (action == "moveTo")
    {
        if (!payload || !(*payload)["position"].is<long>())
        {
            MLOG_WARN("Stepper [%s]: Invalid 'moveTo' payload - need position", _id.c_str());
            return false;
        }

        long position = (*payload)["position"].as<long>();
        float speed = -1;
        float acceleration = -1;

        if (payload && (*payload)["speed"].is<float>())
        {
            speed = (*payload)["speed"].as<float>();
        }

        if (payload && (*payload)["acceleration"].is<float>())
        {
            acceleration = (*payload)["acceleration"].as<float>();
        }

        MLOG_INFO("Stepper [%s]: Move to position %ld (Speed: %.2f, Acceleration: %.2f)", _id.c_str(), position,
                  speed > 0 ? speed : _maxSpeed, acceleration > 0 ? acceleration : _maxAcceleration);
        return moveTo(position, speed, acceleration);
    }
    else if (action == "stop")
    {
        float acceleration = -1;
        if (payload && (*payload)["acceleration"].is<float>())
        {
            acceleration = (*payload)["acceleration"].as<float>();
        }
        return stop(acceleration);
    }
    else if (action == "setCurrentPosition")
    {
        if (!payload || !(*payload)["position"].is<long>())
        {
            MLOG_WARN("Stepper [%s]: Invalid 'setCurrentPosition' payload - need position", _id.c_str());
            return false;
        }

        long position = (*payload)["position"].as<long>();
        MLOG_INFO("Stepper [%s]: Reset position to %ld", _id.c_str(), position);
        return setCurrentPosition(position);
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
    else if (action == "setMaxAcceleration")
    {
        if (!payload || !(*payload)["maxAcceleration"].is<float>())
        {
            Serial.println("Stepper [" + _id + "]: Invalid 'setMaxAcceleration' payload - need maxAcceleration");
            return false;
        }

        float maxAcceleration = (*payload)["maxAcceleration"].as<float>();
        if (maxAcceleration <= 0 || maxAcceleration > 10000)
        {
            Serial.println("Stepper [" + _id + "]: Invalid acceleration " + String(acceleration) + " (range: 1-10000)");
            return false;
        }

        setMaxAcceleration(maxAcceleration);
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
        if (_enablePin >= 0)
        {
            pins.push_back(_enablePin);
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
void Stepper::configure2Pin(int stepPin, int dirPin, int enablePin)
{
    cleanupAccelStepper();

    _is4Pin = false;
    _pin1 = stepPin;
    _pin2 = dirPin;
    _pin3 = -1;
    _pin4 = -1;
    _enablePin = enablePin;
    _stepperType = "DRIVER";
    _interfaceType = AccelStepper::DRIVER;

    initializeAccelStepper();
    _configured = true;

    MLOG_INFO("Stepper [%s]: Configured as DRIVER type on pins %d (step), %d (dir), %d (enable)", _id.c_str(), _pin1, _pin2, _enablePin);

    // Set up enable pin if configured
    if (_enablePin >= 0)
    {
        pinMode(_enablePin, OUTPUT);
        disableStepper(); // Start disabled
    }
}

/**
 * @brief Configure stepper for 4-pin mode (HALF4WIRE or FULL4WIRE)
 * @param pin1 GPIO pin number for motor pin 1
 * @param pin2 GPIO pin number for motor pin 2
 * @param pin3 GPIO pin number for motor pin 3
 * @param pin4 GPIO pin number for motor pin 4
 */
void Stepper::configure4Pin(int pin1, int pin2, int pin3, int pin4,
                            AccelStepper::MotorInterfaceType mode,
                            const char *typeLabel,
                            int enablePin)
{
    cleanupAccelStepper();

    _is4Pin = true;
    _pin1 = pin1;
    _pin2 = pin2;
    _pin3 = pin3;
    _pin4 = pin4;
    _enablePin = enablePin;
    _interfaceType = mode;
    _stepperType = String(typeLabel ? typeLabel : "HALF4WIRE");

    initializeAccelStepper();
    _configured = true;

    MLOG_INFO("Stepper [%s]: Configured as %s type on pins %d, %d, %d, %d, %d (enable)",
              _id.c_str(), _stepperType.c_str(), _pin1, _pin2, _pin3, _pin4, _enablePin);

    // Set up enable pin if configured
    if (_enablePin >= 0)
    {
        pinMode(_enablePin, OUTPUT);
        disableStepper(); // Start disabled
    }
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

    switch (_interfaceType)
    {
    case AccelStepper::FULL4WIRE:
        _stepper = new AccelStepper(AccelStepper::FULL4WIRE, _pin1, _pin3, _pin2, _pin4);
        break;
    case AccelStepper::HALF4WIRE:
        _stepper = new AccelStepper(AccelStepper::HALF4WIRE, _pin1, _pin3, _pin2, _pin4);
        break;
    case AccelStepper::DRIVER:
    default:
        _stepper = new AccelStepper(AccelStepper::DRIVER, _pin1, _pin2);
        break;
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

    config["name"] = _name;
    if (_configured)
    {
        config["stepperType"] = _stepperType;
        config["is4Pin"] = _is4Pin;
        config["maxSpeed"] = _maxSpeed;
        config["maxAcceleration"] = _maxAcceleration;

        if (_is4Pin)
        {
            JsonArray pins = config["pins"].to<JsonArray>();
            pins.add(_pin1);
            pins.add(_pin2);
            pins.add(_pin3);
            pins.add(_pin4);
            if (_enablePin >= 0)
            {
                pins.add(_enablePin);
            }
        }
        else
        {
            JsonArray pins = config["pins"].to<JsonArray>();
            pins.add(_pin1);
            pins.add(_pin2);
            if (_enablePin >= 0)
            {
                pins.add(_enablePin);
            }
        }

        // Add enable pin if configured
        // if (_enablePin >= 0)
        // {
        //     config["enablePin"] = _enablePin;
        //     config["invertEnable"] = _invertEnable;
        // }

        if (_enablePin >= 0)
        {
            config["invertEnable"] = _invertEnable;
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
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Stepper [%s]: Null config provided", _id.c_str());
        return;
    }

    if ((*config)["name"].is<String>())
    {
        _name = (*config)["name"].as<String>();
    }

    // Check if we should configure as 2-pin or 4-pin
    if ((*config)["stepperType"].is<String>())
    {
        String stepperType = (*config)["stepperType"].as<String>();

        if (stepperType == "DRIVER")
        {
            // Configure as 2-pin
            if ((*config)["pins"].is<JsonArray>() && (*config)["pins"].size() >= 2)
            {
                JsonArray pins = (*config)["pins"].as<JsonArray>();
                int stepPin = pins[0].as<int>();
                int dirPin = pins[1].as<int>();
                int enablePin = pins.size() >= 3 ? pins[2].as<int>() : -1;

                configure2Pin(stepPin, dirPin, enablePin);
            }
            else
            {
                MLOG_WARN("Stepper [%s]: Invalid 2-pin configuration in JSON", _id.c_str());
            }
        }
        else if (stepperType == "HALF4WIRE")
        {
            // Configure as 4-pin
            if ((*config)["pins"].is<JsonArray>() && ((*config)["pins"].size() == 4 || (*config)["pins"].size() == 5))
            {
                JsonArray pins = (*config)["pins"].as<JsonArray>();
                int pin1 = pins[0].as<int>();
                int pin2 = pins[1].as<int>();
                int pin3 = pins[2].as<int>();
                int pin4 = pins[3].as<int>();
                int enablePin = pins.size() == 5 ? pins[4].as<int>() : -1;

                configure4Pin(pin1, pin2, pin3, pin4, AccelStepper::HALF4WIRE, "HALF4WIRE", enablePin);
            }
            else
            {
                MLOG_WARN("Stepper [%s]: Invalid 4-pin configuration in JSON", _id.c_str());
            }
        }
        else if (stepperType == "FULL4WIRE")
        {
            if ((*config)["pins"].is<JsonArray>() && ((*config)["pins"].size() == 4 || (*config)["pins"].size() == 5))
            {
                JsonArray pins = (*config)["pins"].as<JsonArray>();
                int pin1 = pins[0].as<int>();
                int pin2 = pins[1].as<int>();
                int pin3 = pins[2].as<int>();
                int pin4 = pins[3].as<int>();
                int enablePin = pins.size() == 5 ? pins[4].as<int>() : -1;

                configure4Pin(pin1, pin2, pin3, pin4, AccelStepper::FULL4WIRE, "FULL4WIRE", enablePin);
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
    // Extract speed and acceleration values first
    float maxSpeed = (*config)["maxSpeed"].is<float>() ? (*config)["maxSpeed"].as<float>() : 1000.0;
    float maxAcceleration = (*config)["maxAcceleration"].is<float>() ? (*config)["maxAcceleration"].as<float>() : ((*config)["acceleration"].is<float>() ? (*config)["acceleration"].as<float>() : 500.0);

    _maxSpeed = maxSpeed;
    _maxAcceleration = maxAcceleration;

    // Load invert enable setting
    if ((*config)["invertEnable"].is<bool>())
    {
        _invertEnable = (*config)["invertEnable"].as<bool>();
    }
    else
    {
        _invertEnable = false; // Default to false
    }

    if (_stepper)
    {
        _stepper->setMaxSpeed(_maxSpeed);
        _stepper->setAcceleration(_maxAcceleration);
    }
}

/**
 * @brief Enable the stepper motor (set enable pin appropriately based on inversion)
 */
void Stepper::enableStepper()
{
    if (_enablePin >= 0)
    {
        digitalWrite(_enablePin, _invertEnable ? LOW : HIGH);
    }
}

/**
 * @brief Disable the stepper motor (set enable pin appropriately based on inversion)
 */
void Stepper::disableStepper()
{
    if (_enablePin >= 0)
    {
        digitalWrite(_enablePin, _invertEnable ? HIGH : LOW);
    }
}
