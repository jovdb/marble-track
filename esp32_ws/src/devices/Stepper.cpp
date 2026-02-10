/**
 * @file Stepper.cpp
 * @brief Stepper implementation using Device and composition mixins
 */

#include "devices/Stepper.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{
    namespace
    {
        PinConfig parsePinConfig(const JsonVariantConst &value)
        {
            PinConfig config;
            if (value.is<int>())
            {
                config.pin = value.as<int>();
                config.expanderId = "";
                return config;
            }

            if (!value.isNull())
            {
                JsonDocument pinDoc;
                pinDoc.set(value);
                return PinFactory::jsonToConfig(pinDoc);
            }

            config.pin = -1;
            config.expanderId = "";
            return config;
        }

        class PinAccelStepper : public AccelStepper
        {
        public:
            PinAccelStepper(uint8_t interface,
                            pins::IPin *pin1,
                            pins::IPin *pin2,
                            pins::IPin *pin3,
                            pins::IPin *pin4,
                            bool invertDirection)
                : AccelStepper(interface, 0, 0, 0, 0, false),
                  _pin1(pin1),
                  _pin2(pin2),
                  _pin3(pin3),
                  _pin4(pin4),
                  _invertDirection(invertDirection)
            {
            }

        protected:
            void setOutputPins(uint8_t mask) override
            {
                if (_pin1)
                    _pin1->write((mask & 0x1) ? HIGH : LOW);
                if (_pin2)
                    _pin2->write((mask & 0x2) ? HIGH : LOW);
                if (_pin3)
                    _pin3->write((mask & 0x4) ? HIGH : LOW);
                if (_pin4)
                    _pin4->write((mask & 0x8) ? HIGH : LOW);
            }

            void step1(long step) override
            {
                (void)step;
                if (!_pin1 || !_pin2)
                    return;

                bool isCw = _direction == DIRECTION_CW;
                if (_invertDirection)
                {
                    isCw = !isCw;
                }
                _pin2->write(isCw ? HIGH : LOW);
                _pin1->write(HIGH);
                delayMicroseconds(1);
                _pin1->write(LOW);
            }

        private:
            pins::IPin *_pin1 = nullptr;
            pins::IPin *_pin2 = nullptr;
            pins::IPin *_pin3 = nullptr;
            pins::IPin *_pin4 = nullptr;
            bool _invertDirection = false;
        };
    }

    Stepper::Stepper(const String &id)
        : Device(id, "stepper")
    {
    }

    Stepper::~Stepper()
    {
        cleanupAccelStepper();
        cleanupPins();
    }

    void Stepper::setup()
    {
        try {
            Device::setup();

            setName(_config.name);

            if (_config.stepperType.isEmpty())
            {
                MLOG_WARN("%s: Stepper type not configured", toString().c_str());
                return;
            }

            // Validate stepper type
            if (_config.stepperType != "DRIVER" && _config.stepperType != "HALF4WIRE" && _config.stepperType != "FULL4WIRE") {
                MLOG_ERROR("%s: Invalid stepper type '%s'. Must be DRIVER, HALF4WIRE, or FULL4WIRE", 
                          toString().c_str(), _config.stepperType.c_str());
                return;
            }

            cleanupPins();

            auto configureOutputPin = [&](const PinConfig &config, pins::IPin *&pin, const char *label, bool required) -> bool
            {
                // Validate pin range and expanderId
                if (config.pin < -1 || config.pin > 50) {
                    if (required) {
                        MLOG_ERROR("%s: %s has invalid pin value %d (must be 0-50 or -1)", toString().c_str(), label, config.pin);
                        return false;
                    }
                    return true;
                }
                
                if (config.pin < 0)
                {
                    if (required)
                    {
                        MLOG_WARN("%s: %s not configured", toString().c_str(), label);
                        return false;
                    }
                    return true;
                }

                pin = PinFactory::createPin(config);
                if (!pin)
                {
                    MLOG_ERROR("%s: Failed to create %s %s", toString().c_str(), label, config.toString().c_str());
                    return false;
                }

                if (!pin->setup(config.pin, pins::PinMode::Output))
                {
                    MLOG_ERROR("%s: Failed to setup %s %s", toString().c_str(), label, config.toString().c_str());
                    return false;
                }

                return true;
            };

            if (_config.stepperType == "DRIVER")
            {
                if (!configureOutputPin(_config.stepPin, _stepPin, "step pin", true))
                {
                    cleanupPins();
                    return;
                }
                if (!configureOutputPin(_config.dirPin, _dirPin, "direction pin", true))
                {
                    cleanupPins();
                    return;
                }
            }
            else if (_config.stepperType == "HALF4WIRE" || _config.stepperType == "FULL4WIRE")
            {
                if (!configureOutputPin(_config.pin1, _pin1, "pin1", true))
                {
                    cleanupPins();
                    return;
                }
                if (!configureOutputPin(_config.pin2, _pin2, "pin2", true))
                {
                    cleanupPins();
                    return;
                }
                if (!configureOutputPin(_config.pin3, _pin3, "pin3", true))
                {
                    cleanupPins();
                    return;
                }
                if (!configureOutputPin(_config.pin4, _pin4, "pin4", true))
                {
                    cleanupPins();
                    return;
                }
            }

            if (!configureOutputPin(_config.enablePin, _enablePin, "enable pin", false))
            {
                cleanupPins();
                return;
            }

            initializeAccelStepper();

            if (_driver)
            {
                // Validate float values before passing to AccelStepper
                float maxSpeed = _config.maxSpeed;
                float maxAccel = _config.maxAcceleration;
                
                if (isnan(maxSpeed) || isinf(maxSpeed) || maxSpeed <= 0 || maxSpeed > 100000) {
                    MLOG_WARN("%s: Invalid maxSpeed %.2f, using default 1000", toString().c_str(), maxSpeed);
                    maxSpeed = 1000.0f;
                }
                
                if (isnan(maxAccel) || isinf(maxAccel) || maxAccel <= 0 || maxAccel > 100000) {
                    MLOG_WARN("%s: Invalid maxAcceleration %.2f, using default 1000", toString().c_str(), maxAccel);
                    maxAccel = 1000.0f;
                }
                
                _driver->setMaxSpeed(maxSpeed);
                _driver->setAcceleration(maxAccel);
                _driver->setCurrentPosition(0);

                if (_enablePin && _enablePin->isConfigured())
                {
                    disableStepper();
                }

                // Log all pins used
                std::vector<String> pins = getPins();
                String pinStr = "";
                if (!pins.empty())
                {
                    for (size_t i = 0; i < pins.size(); i++)
                    {
                        if (i > 0)
                            pinStr += ", ";
                        pinStr += pins[i];
                    }
                }
                MLOG_INFO("%s: Setup complete on pins %s, type: %s", toString().c_str(), pinStr.c_str(), _config.stepperType.c_str());
            }
            else
            {
                MLOG_ERROR("%s: Failed to initialize AccelStepper", toString().c_str());
            }
        } catch (const std::exception& e) {
            MLOG_ERROR("%s: Exception during setup: %s", toString().c_str(), e.what());
            cleanupPins();
            cleanupAccelStepper();
        } catch (...) {
            MLOG_ERROR("%s: Unknown exception during setup", toString().c_str());
            cleanupPins();
            cleanupAccelStepper();
        }
    }

    void Stepper::teardown()
    {
        Device::teardown();

        if (_config.stepPin.expanderId.isEmpty() && _config.stepPin.pin >= 0)
            pinMode(_config.stepPin.pin, INPUT);
        if (_config.dirPin.expanderId.isEmpty() && _config.dirPin.pin >= 0)
            pinMode(_config.dirPin.pin, INPUT);
        if (_config.pin1.expanderId.isEmpty() && _config.pin1.pin >= 0)
            pinMode(_config.pin1.pin, INPUT);
        if (_config.pin2.expanderId.isEmpty() && _config.pin2.pin >= 0)
            pinMode(_config.pin2.pin, INPUT);
        if (_config.pin3.expanderId.isEmpty() && _config.pin3.pin >= 0)
            pinMode(_config.pin3.pin, INPUT);
        if (_config.pin4.expanderId.isEmpty() && _config.pin4.pin >= 0)
            pinMode(_config.pin4.pin, INPUT);
        if (_config.enablePin.expanderId.isEmpty() && _config.enablePin.pin >= 0)
            pinMode(_config.enablePin.pin, INPUT);

        cleanupAccelStepper();
        cleanupPins();

        _state.isMoving = false;
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

    std::vector<String> Stepper::getPins() const
    {
        std::vector<String> pins;
        if (_config.stepperType == "DRIVER")
        {
            if (_stepPin && _stepPin->isConfigured())
                pins.push_back(_stepPin->toString());
            if (_dirPin && _dirPin->isConfigured())
                pins.push_back(_dirPin->toString());
        }
        else if (_config.stepperType == "HALF4WIRE" || _config.stepperType == "FULL4WIRE")
        {
            if (_pin1 && _pin1->isConfigured())
                pins.push_back(_pin1->toString());
            if (_pin2 && _pin2->isConfigured())
                pins.push_back(_pin2->toString());
            if (_pin3 && _pin3->isConfigured())
                pins.push_back(_pin3->toString());
            if (_pin4 && _pin4->isConfigured())
                pins.push_back(_pin4->toString());
        }
        if (_enablePin && _enablePin->isConfigured())
            pins.push_back(_enablePin->toString());
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

        MLOG_INFO("%s: Started moving %ld steps at %f steps/s, accel %f steps/s²", toString().c_str(), steps, speed, acceleration);
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

        MLOG_INFO("%s: Started moving to position %ld at %f steps/s, accel %f steps/s²", toString().c_str(), position, speed, acceleration);
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
        // Don't set _state.isMoving = false here, let the loop handle it
        return true;
    }

    bool Stepper::setCurrentPosition(long position)
    {
        if (!ensureReady("setCurrentPosition"))
            return false;

        _driver->setCurrentPosition(position);

        _state.currentPosition = position;
        _state.targetPosition = position;

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
        if (!config["stepPin"].isNull())
            _config.stepPin = parsePinConfig(config["stepPin"]);
        if (!config["dirPin"].isNull())
            _config.dirPin = parsePinConfig(config["dirPin"]);
        if (!config["pin1"].isNull())
            _config.pin1 = parsePinConfig(config["pin1"]);
        if (!config["pin2"].isNull())
            _config.pin2 = parsePinConfig(config["pin2"]);
        if (!config["pin3"].isNull())
            _config.pin3 = parsePinConfig(config["pin3"]);
        if (!config["pin4"].isNull())
            _config.pin4 = parsePinConfig(config["pin4"]);
        if (!config["enablePin"].isNull())
            _config.enablePin = parsePinConfig(config["enablePin"]);
        if (config["invertEnable"].is<bool>())
            _config.invertEnable = config["invertEnable"].as<bool>();
        if (config["invertDirection"].is<bool>())
            _config.invertDirection = config["invertDirection"].as<bool>();
    }

    void Stepper::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["stepperType"] = _config.stepperType;
        doc["maxSpeed"] = _config.maxSpeed;
        doc["maxAcceleration"] = _config.maxAcceleration;
        doc["defaultSpeed"] = _config.defaultSpeed;
        doc["defaultAcceleration"] = _config.defaultAcceleration;
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.stepPin, pinDoc);
            doc["stepPin"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.dirPin, pinDoc);
            doc["dirPin"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.pin1, pinDoc);
            doc["pin1"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.pin2, pinDoc);
            doc["pin2"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.pin3, pinDoc);
            doc["pin3"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.pin4, pinDoc);
            doc["pin4"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.enablePin, pinDoc);
            doc["enablePin"] = pinDoc.as<JsonVariant>();
        }
        doc["invertEnable"] = _config.invertEnable;
        doc["invertDirection"] = _config.invertDirection;
    }

    void Stepper::initializeAccelStepper()
    {
        cleanupAccelStepper();

        if (_config.stepperType == "DRIVER")
        {
            if (!_stepPin || !_dirPin)
            {
                MLOG_ERROR("%s: Stepper DRIVER pins not configured", toString().c_str());
                _driver = nullptr;
                return;
            }
            _driver = new PinAccelStepper(AccelStepper::DRIVER, _stepPin, _dirPin, nullptr, nullptr, _config.invertDirection);
        }
        else if (_config.stepperType == "HALF4WIRE")
        {
            if (!_pin1 || !_pin2 || !_pin3 || !_pin4)
            {
                MLOG_ERROR("%s: Stepper HALF4WIRE pins not configured", toString().c_str());
                _driver = nullptr;
                return;
            }
            _driver = new PinAccelStepper(AccelStepper::HALF4WIRE, _pin1, _pin3, _pin2, _pin4, false);
        }
        else if (_config.stepperType == "FULL4WIRE")
        {
            if (!_pin1 || !_pin2 || !_pin3 || !_pin4)
            {
                MLOG_ERROR("%s: Stepper FULL4WIRE pins not configured", toString().c_str());
                _driver = nullptr;
                return;
            }
            _driver = new PinAccelStepper(AccelStepper::FULL4WIRE, _pin1, _pin3, _pin2, _pin4, false);
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

    void Stepper::cleanupPins()
    {
        // Safe deletion with additional checks
        if (_stepPin) {
            try {
                delete _stepPin;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _stepPin", toString().c_str());
            }
            _stepPin = nullptr;
        }
        if (_dirPin) {
            try {
                delete _dirPin;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _dirPin", toString().c_str());
            }
            _dirPin = nullptr;
        }
        if (_pin1) {
            try {
                delete _pin1;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _pin1", toString().c_str());
            }
            _pin1 = nullptr;
        }
        if (_pin2) {
            try {
                delete _pin2;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _pin2", toString().c_str());
            }
            _pin2 = nullptr;
        }
        if (_pin3) {
            try {
                delete _pin3;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _pin3", toString().c_str());
            }
            _pin3 = nullptr;
        }
        if (_pin4) {
            try {
                delete _pin4;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _pin4", toString().c_str());
            }
            _pin4 = nullptr;
        }
        if (_enablePin) {
            try {
                delete _enablePin;
            } catch (...) {
                MLOG_ERROR("%s: Exception deleting _enablePin", toString().c_str());
            }
            _enablePin = nullptr;
        }
    }

    void Stepper::enableStepper()
    {
        if (_enablePin && _enablePin->isConfigured())
            _enablePin->write(_config.invertEnable ? LOW : HIGH);
    }

    void Stepper::disableStepper()
    {
        if (_enablePin && _enablePin->isConfigured())
            _enablePin->write(_config.invertEnable ? HIGH : LOW);
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
