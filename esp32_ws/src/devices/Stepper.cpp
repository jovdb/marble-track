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
        _stateMutex = xSemaphoreCreateMutex();
    }

    Stepper::~Stepper()
    {
        cleanupAccelStepper();
        cleanupPins();
        if (_stateMutex != nullptr)
        {
            vSemaphoreDelete(_stateMutex);
        }
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

        cleanupPins();

        auto configureOutputPin = [&](const PinConfig &config, pins::IPin *&pin, const char *label, bool required) -> bool {
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
            _driver->setMaxSpeed(_config.maxSpeed);
            _driver->setAcceleration(_config.maxAcceleration);
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

            if (!startTask("StepperTask", 4096, 1, 1))
            {
                MLOG_ERROR("%s: Failed to start RTOS task", toString().c_str());
            }
        }
        else
        {
            MLOG_ERROR("%s: Failed to initialize AccelStepper", toString().c_str());
        }
    }

    void Stepper::loop()
    {
        Device::loop();
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

        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.moveCommand.pending = true;
        _state.moveCommand.type = "move";
        _state.moveCommand.steps = steps;
        _state.moveCommand.speed = speed;
        _state.moveCommand.acceleration = acceleration;
        xSemaphoreGive(_stateMutex);

        notifyTask();
        return true;
    }

    bool Stepper::moveTo(long position, float speed, float acceleration)
    {
        if (!ensureReady("moveTo"))
            return false;

        prepareForMove(speed, acceleration);

        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.moveCommand.pending = true;
        _state.moveCommand.type = "moveTo";
        _state.moveCommand.position = position;
        _state.moveCommand.speed = speed;
        _state.moveCommand.acceleration = acceleration;
        xSemaphoreGive(_stateMutex);

        notifyTask();
        return true;
    }

    bool Stepper::stop(float acceleration)
    {
        if (!ensureReady("stop"))
            return false;

        if (acceleration <= 0)
            acceleration = _config.defaultAcceleration;

        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.moveCommand.pending = true;
        _state.moveCommand.type = "stop";
        _state.moveCommand.acceleration = acceleration;
        xSemaphoreGive(_stateMutex);

        notifyTask();
        return true;
    }

    bool Stepper::setCurrentPosition(long position)
    {
        if (!ensureReady("setCurrentPosition"))
            return false;

        _driver->setCurrentPosition(position);

        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.currentPosition = position;
        xSemaphoreGive(_stateMutex);

        notifyStateChanged();
        return true;
    }

    void Stepper::addStateToJson(JsonDocument &doc)
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {
            doc["currentPosition"] = _state.currentPosition;
            doc["targetPosition"] = _state.targetPosition;
            doc["isMoving"] = _state.isMoving;
            xSemaphoreGive(_stateMutex);
        }
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

    void Stepper::task()
    {
        MLOG_DEBUG("%s: RTOS task started", toString().c_str());

        while (true)
        {
            // Wait for command or periodic check
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));

            // Handle pending commands
            xSemaphoreTake(_stateMutex, portMAX_DELAY);
            if (_state.moveCommand.pending)
            {
                MoveCommand cmd = _state.moveCommand;
                _state.moveCommand.pending = false;
                xSemaphoreGive(_stateMutex);

                if (cmd.type == "move")
                {
                    enableStepper();
                    float actualSpeed = cmd.speed > 0 ? cmd.speed : _config.defaultSpeed;
                    float actualAcceleration = cmd.acceleration > 0 ? cmd.acceleration : _config.defaultAcceleration;
                    _driver->setMaxSpeed(actualSpeed);
                    _driver->setAcceleration(actualAcceleration);
                    _driver->move(cmd.steps);

                    xSemaphoreTake(_stateMutex, portMAX_DELAY);
                    _state.isMoving = true;
                    _state.moveJustStarted = true;
                    _state.currentPosition = _driver->currentPosition();
                    _state.targetPosition = _driver->targetPosition();
                    xSemaphoreGive(_stateMutex);

                    MLOG_INFO("%s: Started moving %ld steps at %f steps/s, accel %f steps/s²", toString().c_str(), cmd.steps, actualSpeed, actualAcceleration);
                    notifyStateChanged();
                }
                else if (cmd.type == "moveTo")
                {
                    enableStepper();
                    float actualSpeed = cmd.speed > 0 ? cmd.speed : _config.defaultSpeed;
                    float actualAcceleration = cmd.acceleration > 0 ? cmd.acceleration : _config.defaultAcceleration;
                    _driver->setMaxSpeed(actualSpeed);
                    _driver->setAcceleration(actualAcceleration);
                    _driver->moveTo(cmd.position);

                    xSemaphoreTake(_stateMutex, portMAX_DELAY);
                    _state.isMoving = true;
                    _state.moveJustStarted = true;
                    _state.currentPosition = _driver->currentPosition();
                    _state.targetPosition = cmd.position;
                    xSemaphoreGive(_stateMutex);

                    MLOG_INFO("%s: Started moving to position %ld at %f steps/s, accel %f steps/s²", toString().c_str(), cmd.position, actualSpeed, actualAcceleration);
                    notifyStateChanged();
                }
                else if (cmd.type == "stop")
                {
                    _driver->setAcceleration(cmd.acceleration > 0 ? cmd.acceleration : _config.defaultAcceleration);
                    _driver->stop();
                    // Don't set _state.isMoving = false here, let the run loop handle it
                }
            }
            else
            {
                xSemaphoreGive(_stateMutex);
            }

            // Run the stepper
            if (_driver)
            {
                xSemaphoreTake(_stateMutex, portMAX_DELAY);
                bool wasMoving = _state.isMoving;
                bool moveJustStarted = _state.moveJustStarted;
                xSemaphoreGive(_stateMutex);

                if (moveJustStarted)
                {
                    xSemaphoreTake(_stateMutex, portMAX_DELAY);
                    _state.moveJustStarted = false;
                    xSemaphoreGive(_stateMutex);
                }
                else
                {
                    bool isRunning = _driver->run();

                    xSemaphoreTake(_stateMutex, portMAX_DELAY);
                    _state.isMoving = isRunning;
                    _state.currentPosition = _driver->currentPosition();
                    xSemaphoreGive(_stateMutex);

                    if (wasMoving && !isRunning)
                    {
                        disableStepper();
                        MLOG_INFO("%s: Movement completed at position %ld", toString().c_str(), _driver->currentPosition());
                        notifyStateChanged();
                    }
                }
            }
        }
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
        if (_stepPin)
        {
            delete _stepPin;
            _stepPin = nullptr;
        }
        if (_dirPin)
        {
            delete _dirPin;
            _dirPin = nullptr;
        }
        if (_pin1)
        {
            delete _pin1;
            _pin1 = nullptr;
        }
        if (_pin2)
        {
            delete _pin2;
            _pin2 = nullptr;
        }
        if (_pin3)
        {
            delete _pin3;
            _pin3 = nullptr;
        }
        if (_pin4)
        {
            delete _pin4;
            _pin4 = nullptr;
        }
        if (_enablePin)
        {
            delete _enablePin;
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
