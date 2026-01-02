/**
 * @file Stepper.cpp
 * @brief Stepper implementation using Device and composition mixins
 */

#include "devices/Stepper.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    Stepper::Stepper(const String &id)
        : Device(id, "stepper")
    {
    }

    Stepper::~Stepper()
    {
        cleanupAccelStepper();
    }

    void Stepper::setup()
    {
        Device::setup();

        setName(_config.name);

        if (_config.stepperType.isEmpty())
        {
            MLOG_WARN("%s: Stepper type not configured", toString().c_str());
            return;
        }

        initializeAccelStepper();

        if (_driver)
        {
            _driver->setMaxSpeed(_config.maxSpeed);
            _driver->setAcceleration(_config.maxAcceleration);
            _driver->setCurrentPosition(0);

            if (_config.enablePin >= 0)
            {
                pinMode(_config.enablePin, OUTPUT);
                disableStepper();
            }

            // Log all pins used
            std::vector<int> pins = getPins();
            String pinStr = "";
            if (!pins.empty())
            {
                for (size_t i = 0; i < pins.size(); i++)
                {
                    if (i > 0)
                        pinStr += ", ";
                    pinStr += String(pins[i]);
                }
            }
            MLOG_INFO("%s: Setup complete on pins %s, type: %s", toString().c_str(), pinStr.c_str(), _config.stepperType.c_str());
        }
        else
        {
            MLOG_ERROR("%s: Failed to initialize AccelStepper", toString().c_str());
        }
    }

    void Stepper::loop()
    {
        Device::loop();

        if (_driver)
        {
            bool wasMoving = _state.isMoving;
            bool isRunning = _driver->run();

            _state.isMoving = isRunning;
            _state.currentPosition = _driver->currentPosition();

            if (wasMoving && !isRunning)
            {
                disableStepper();
                MLOG_INFO("%s: Movement completed at position %ld", toString().c_str(), _driver->currentPosition());
                notifyStateChanged();
            }
        }
    }

    std::vector<int> Stepper::getPins() const
    {
        std::vector<int> pins;
        if (_config.stepperType == "DRIVER")
        {
            if (_config.stepPin >= 0)
                pins.push_back(_config.stepPin);
            if (_config.dirPin >= 0)
                pins.push_back(_config.dirPin);
        }
        else if (_config.stepperType == "HALF4WIRE" || _config.stepperType == "FULL4WIRE")
        {
            if (_config.pin1 >= 0)
                pins.push_back(_config.pin1);
            if (_config.pin2 >= 0)
                pins.push_back(_config.pin2);
            if (_config.pin3 >= 0)
                pins.push_back(_config.pin3);
            if (_config.pin4 >= 0)
                pins.push_back(_config.pin4);
        }
        if (_config.enablePin >= 0)
            pins.push_back(_config.enablePin);
        return pins;
    }

    bool Stepper::move(long steps, float speed, float acceleration)
    {
        if (!ensureReady("move"))
            return false;

        prepareForMove(speed, acceleration);

        enableStepper();
        _driver->setMaxSpeed(speed);
        _driver->setAcceleration(acceleration);
        _driver->move(steps);

        _state.isMoving = true;
        _state.currentPosition = _driver->currentPosition();
        _state.targetPosition = _driver->targetPosition();

        MLOG_INFO("%s: Started moving %ld steps at %.1f steps/sec, accel %.1f steps/sec²", toString().c_str(), steps, speed, acceleration);
        notifyStateChanged();
        return true;
    }

    bool Stepper::moveTo(long position, float speed, float acceleration)
    {
        if (!ensureReady("moveTo"))
            return false;

        prepareForMove(speed, acceleration);

        enableStepper();
        _driver->setMaxSpeed(speed);
        _driver->setAcceleration(acceleration);
        _driver->moveTo(position);

        _state.isMoving = true;
        _state.currentPosition = _driver->currentPosition();
        _state.targetPosition = position;

        MLOG_INFO("%s: Started moving to position %ld at %.1f steps/sec, accel %.1f steps/sec²", toString().c_str(), position, speed, acceleration);
        notifyStateChanged();
        return true;
    }

    bool Stepper::stop(float acceleration)
    {
        if (!ensureReady("stop"))
            return false;

        if (acceleration <= 0)
            acceleration = _config.defaultAcceleration;

        _driver->setAcceleration(acceleration);
        _driver->stop();
        // Don't set _state.isMoving = false here, let the run loop handle it
        return true;
    }

    bool Stepper::setCurrentPosition(long position)
    {
        if (!ensureReady("setCurrentPosition"))
            return false;

        _driver->setCurrentPosition(position);
        _state.currentPosition = position;

        notifyStateChanged();
        return true;
    }

    void Stepper::addStateToJson(JsonDocument &doc)
    {
        doc["currentPosition"] = _state.currentPosition;
        doc["targetPosition"] = _state.targetPosition;
        doc["isMoving"] = _state.isMoving;
    }

    bool Stepper::control(const String &action, JsonObject *args)
    {
        if (action == "move")
        {
            if (!args || !(*args)["steps"].is<long>())
                return false;
            long steps = (*args)["steps"].as<long>();
            float speed = (*args)["speed"] | -1.0f;
            float acceleration = (*args)["acceleration"] | -1.0f;
            return move(steps, speed, acceleration);
        }
        else if (action == "moveTo")
        {
            if (!args || !(*args)["position"].is<long>())
                return false;
            long position = (*args)["position"].as<long>();
            float speed = (*args)["speed"] | -1.0f;
            float acceleration = (*args)["acceleration"] | -1.0f;
            return moveTo(position, speed, acceleration);
        }
        else if (action == "stop")
        {
            float acceleration = args && (*args)["acceleration"].is<float>() ? (*args)["acceleration"].as<float>() : -1.0f;
            return stop(acceleration);
        }
        else if (action == "setCurrentPosition")
        {
            if (!args || !(*args)["position"].is<long>())
                return false;
            long position = (*args)["position"].as<long>();
            return setCurrentPosition(position);
        }
        else
        {
            MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Stepper::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["stepperType"].is<String>())
            _config.stepperType = config["stepperType"].as<String>();
        if (config["maxSpeed"].is<float>())
            _config.maxSpeed = config["maxSpeed"].as<float>();
        if (config["maxAcceleration"].is<float>())
            _config.maxAcceleration = config["maxAcceleration"].as<float>();
        if (config["defaultSpeed"].is<float>())
            _config.defaultSpeed = config["defaultSpeed"].as<float>();
        if (config["defaultAcceleration"].is<float>())
            _config.defaultAcceleration = config["defaultAcceleration"].as<float>();
        if (config["stepPin"].is<int>())
            _config.stepPin = config["stepPin"].as<int>();
        if (config["dirPin"].is<int>())
            _config.dirPin = config["dirPin"].as<int>();
        if (config["pin1"].is<int>())
            _config.pin1 = config["pin1"].as<int>();
        if (config["pin2"].is<int>())
            _config.pin2 = config["pin2"].as<int>();
        if (config["pin3"].is<int>())
            _config.pin3 = config["pin3"].as<int>();
        if (config["pin4"].is<int>())
            _config.pin4 = config["pin4"].as<int>();
        if (config["enablePin"].is<int>())
            _config.enablePin = config["enablePin"].as<int>();
        if (config["invertEnable"].is<bool>())
            _config.invertEnable = config["invertEnable"].as<bool>();
    }

    void Stepper::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["stepperType"] = _config.stepperType;
        doc["maxSpeed"] = _config.maxSpeed;
        doc["maxAcceleration"] = _config.maxAcceleration;
        doc["defaultSpeed"] = _config.defaultSpeed;
        doc["defaultAcceleration"] = _config.defaultAcceleration;
        doc["stepPin"] = _config.stepPin;
        doc["dirPin"] = _config.dirPin;
        doc["pin1"] = _config.pin1;
        doc["pin2"] = _config.pin2;
        doc["pin3"] = _config.pin3;
        doc["pin4"] = _config.pin4;
        doc["enablePin"] = _config.enablePin;
        doc["invertEnable"] = _config.invertEnable;
    }

    void Stepper::initializeAccelStepper()
    {
        cleanupAccelStepper();

        if (_config.stepperType == "DRIVER")
        {
            _driver = new AccelStepper(AccelStepper::DRIVER, _config.stepPin, _config.dirPin);
        }
        else if (_config.stepperType == "HALF4WIRE")
        {
            _driver = new AccelStepper(AccelStepper::HALF4WIRE, _config.pin1, _config.pin3, _config.pin2, _config.pin4);
        }
        else if (_config.stepperType == "FULL4WIRE")
        {
            _driver = new AccelStepper(AccelStepper::FULL4WIRE, _config.pin1, _config.pin3, _config.pin2, _config.pin4);
        }
        else
        {
            MLOG_ERROR("%s: Unknown stepper type: %s", toString().c_str(), _config.stepperType.c_str());
            _driver = nullptr;
        }
    }

    void Stepper::cleanupAccelStepper()
    {
        if (_driver)
        {
            delete _driver;
            _driver = nullptr;
        }
    }

    void Stepper::enableStepper()
    {
        if (_config.enablePin >= 0)
        {
            digitalWrite(_config.enablePin, _config.invertEnable ? LOW : HIGH);
        }
    }

    void Stepper::disableStepper()
    {
        if (_config.enablePin >= 0)
        {
            digitalWrite(_config.enablePin, _config.invertEnable ? HIGH : LOW);
        }
    }

    void Stepper::prepareForMove(float &speed, float &acceleration)
    {
        if (speed <= 0)
            speed = _config.defaultSpeed;
        if (speed > _config.maxSpeed)
            speed = _config.maxSpeed;
        if (acceleration <= 0)
            acceleration = _config.defaultAcceleration;
        if (acceleration > _config.maxAcceleration)
            acceleration = _config.maxAcceleration;
    }

    bool Stepper::ensureReady(const char *action, bool logWarning) const
    {
        if (!_driver)
        {
            if (logWarning && action)
                MLOG_WARN("%s: Stepper not initialized - cannot %s", toString().c_str(), action);
            return false;
        }
        return true;
    }

} // namespace devices
