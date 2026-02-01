/**
 * @file Wheel.cpp
 * @brief Wheel implementation using Device and composition mixins
 */

#include "devices/Wheel.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include <cstdlib>

namespace devices
{

    // Default breakpoints (same as original)
    static const float defaultBreakpoints[] = {45.0, 90.0, 180.0, 30.0, 270.0};

    Wheel::Wheel(const String &id)
        : Device(id, "wheel")
    {
        // Create stepper child
        _stepper = new Stepper(getId() + "-stepper");
        addChild(_stepper);

        // Create zero sensor button
        _zeroSensor = new Button(getId() + "-zero-sensor");
        addChild(_zeroSensor);

        // Set default config for stepper
        JsonDocument stepperConfig;
        stepperConfig["name"] = "Wheel Stepper";
        stepperConfig["stepperType"] = "DRIVER";
        stepperConfig["maxSpeed"] = 3000;
        stepperConfig["maxAcceleration"] = 3000;
        stepperConfig["defaultSpeed"] = 1000;
        stepperConfig["defaultAcceleration"] = 200;

        _stepper->jsonToConfig(stepperConfig);

        // Set default config for zero sensor
        JsonDocument sensorConfig;
        sensorConfig["name"] = "Wheel Zero Sensor";
        sensorConfig["pinMode"] = "pullup";
        sensorConfig["debounceMs"] = 50;
        sensorConfig["buttonType"] = "NormalOpen";
        _zeroSensor->jsonToConfig(sensorConfig);
    }

    Wheel::~Wheel()
    {
    }

    void Wheel::setup()
    {
        Device::setup();

        setName(_config.name);

        // Initialize breakpoints if not configured
        if (_config.breakPoints.empty())
        {
            _config.breakPoints.assign(std::begin(defaultBreakpoints), std::end(defaultBreakpoints));
        }

        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void Wheel::teardown()
    {
        Device::teardown();

        _state.state = WheelStateEnum::UNKNOWN;
        _state.errorCode = WheelErrorCode::None;
        _state.errorMessage = "";
        _state.lastZeroPosition = 0;
        _state.stepsInLastRevolution = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _state.targetAngle = -1.0f;
        _state.onError = false;
        _state.breakpointChanged = false;
        _state.zeroSensorWasPressed = false;
        _waitingForMoveStart = false;
        _moveHasStarted = false;
    }

    void Wheel::loop()
    {
        Device::loop();

        if (!_stepper || !_zeroSensor)
        {
            return;
        }

        // Reset error flag
        if (_state.onError)
        {
            _state.onError = false;
        }

        // Reset breakpoint changed flag
        if (_state.breakpointChanged)
        {
            _state.breakpointChanged = false;
        }

        // Get stepper state (assuming Stepper has isMoving method)
        // For now, we'll use a simple approach - in real implementation,
        // we'd need to check stepper's state

        switch (_state.state)
        {
        case WheelStateEnum::IDLE:
        {
            // Idle state - no specific actions needed
            break;
        }
        case WheelStateEnum::MOVING:
        {
            auto stepperState = _stepper->getState();

            if (_waitingForMoveStart && (stepperState.isMoving || stepperState.moveJustStarted))
            {
                _waitingForMoveStart = false;
                _moveHasStarted = true;
            }

            // Check if movement completed (only after movement has actually started)
            if (!_waitingForMoveStart && _moveHasStarted && !stepperState.isMoving)
            {
                MLOG_INFO("%s: Movement to target completed", toString().c_str());

                if (_state.targetBreakpointIndex >= 0)
                {
                    _state.currentBreakpointIndex = _state.targetBreakpointIndex;
                    _state.targetBreakpointIndex = -1;
                    _state.breakpointChanged = true;
                }

                _state.state = WheelStateEnum::IDLE;
                _moveHasStarted = false;
                notifyStateChanged();
            }

            // Check zero sensor for position tracking
            bool zeroPressed = _zeroSensor->getState().isPressed;
            if (zeroPressed && !_state.zeroSensorWasPressed)
            {
                // Zero sensor triggered - update position tracking
                long currentPosition = _stepper->getState().currentPosition;
                _state.stepsInLastRevolution = currentPosition - _state.lastZeroPosition;
                _state.lastZeroPosition = currentPosition;

                // Check revolution consistency
                if (_config.stepsPerRevolution > 0 && _state.stepsInLastRevolution > 0)
                {
                    float percentDiff = abs(_state.stepsInLastRevolution - _config.stepsPerRevolution) / (float)_config.stepsPerRevolution * 100.0f;
                    if (percentDiff > 0.1f)
                    {
                        MLOG_ERROR("%s: Steps per revolution mismatch - measured: %ld, configured: %ld (%.2f%% difference)",
                                   toString().c_str(), _state.stepsInLastRevolution, _config.stepsPerRevolution, percentDiff);
                    }
                }
            }
            else
            {
                const long currentPosition = _stepper->getState().currentPosition;
                const long stepsSinceZero = labs(currentPosition - _state.lastZeroPosition);
                if (_config.maxStepsPerRevolution > 0 && stepsSinceZero >= _config.maxStepsPerRevolution)
                {
                    setErrorState(WheelErrorCode::ZeroNotFound,
                                  "Zero sensor not triggered within maxStepsPerRevolution");
                    notifyStateChanged();
                    break;
                }
            }
            _state.zeroSensorWasPressed = zeroPressed;

            break;
        }
        case WheelStateEnum::INIT:
        {
            // Check for zero sensor trigger
            bool zeroPressed = _zeroSensor->getState().isPressed;
            if (zeroPressed && !_state.zeroSensorWasPressed)
            {
                // Zero sensor triggered - stop and set position
                long currentPosition = _stepper->getState().currentPosition;
                _state.lastZeroPosition = currentPosition;
                _stepper->stop();

                // Move to first breakpoint if configured
                if (!_config.breakPoints.empty() && _config.stepsPerRevolution > 0)
                {
                    MLOG_INFO("%s: Init: Zero point reached at %ld, moving to first breakpoint...", toString().c_str(), currentPosition);
                    _state.currentBreakpointIndex = -1;
                    _state.targetBreakpointIndex = 0;
                    _state.state = WheelStateEnum::MOVING;
                    _state.targetAngle = _config.breakPoints[0];
                    moveToAngle(_config.breakPoints[0]);
                    notifyStateChanged();
                }
                else
                {
                    MLOG_INFO("%s: Init: Zero point reached at %ld, no breakpoints configured", toString().c_str(), currentPosition);
                    _state.state = WheelStateEnum::IDLE;
                    notifyStateChanged();
                }
            }
            else if ((millis() - _initStartTime > 300) && !_stepper->getState().isMoving)
            {
                // Movement completed without finding zero - error
                setErrorState(WheelErrorCode::CalibrationZeroNotFound, "Init: Zero sensor not found!");
                notifyStateChanged();
            }
            _state.zeroSensorWasPressed = zeroPressed;

            break;
        }
        case WheelStateEnum::CALIBRATING:
        {
            // Check for zero sensor trigger
            bool zeroPressed = _zeroSensor->getState().isPressed;
            if (zeroPressed && !_state.zeroSensorWasPressed)
            {
                // Rising edge of zero sensor
                long currentPosition = _stepper->getState().currentPosition;
                if (_state.lastZeroPosition == 0)
                {
                    // First zero trigger - record position
                    _state.lastZeroPosition = currentPosition;
                    MLOG_INFO("%s: Calibration: first zero at position %ld, start counting steps...", toString().c_str(), _state.lastZeroPosition);
                }
                else
                {
                    // Second zero trigger - calculate steps per revolution
                    long steps = currentPosition - _state.lastZeroPosition;
                    _state.stepsInLastRevolution = steps;
                    _config.stepsPerRevolution = steps;

                    // Stop the stepper
                    _stepper->stop();

                    // Set state to IDLE
                    _state.state = WheelStateEnum::IDLE;
                    notifyStateChanged();

                    notifyStepsPerRevolution(steps);

                    MLOG_INFO("%s: Calibration complete, steps per revolution: %ld", toString().c_str(), steps);
                }
            }
            _state.zeroSensorWasPressed = zeroPressed;

            if (!_stepper->getState().isMoving)
            {
                // Movement completed without finding zero - error
                if (_state.lastZeroPosition == 0)
                {
                    setErrorState(WheelErrorCode::CalibrationZeroNotFound, "Calibration: No zero sensor not found!");
                }
                else
                {
                    setErrorState(WheelErrorCode::CalibrationSecondZeroNotFound, "Calibration: Second zero sensor trigger not detected!");
                }
                notifyStateChanged();
            }
            break;
        }
        default:
            break;
        }
    }

    bool Wheel::move(long steps)
    {
        if (_state.state != WheelStateEnum::CALIBRATING && _state.state != WheelStateEnum::INIT)
        {
            MLOG_INFO("%s: Moving %ld steps", toString().c_str(), steps);
            _state.state = WheelStateEnum::MOVING;
            _waitingForMoveStart = true;
            _moveHasStarted = false;
            notifyStateChanged();
        }

        // Call stepper's move method
        return _stepper->move(steps);
    }

    bool Wheel::calibrate(long maxStepsPerRevolution)
    {
        MLOG_INFO("%s: Calibration started", toString().c_str());
        _state.state = WheelStateEnum::CALIBRATING;
        _state.lastZeroPosition = 0;
        _state.stepsInLastRevolution = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _state.zeroSensorWasPressed = _zeroSensor->getState().isPressed; // Initialize to current state
        notifyStateChanged();

        const long maxSteps = (maxStepsPerRevolution > 0) ? maxStepsPerRevolution : _config.maxStepsPerRevolution;
        // Move large number of steps to complete at least one revolution
        return move(maxSteps * 2 * _config.direction);
    }

    bool Wheel::init(long maxStepsPerRevolution)
    {
        MLOG_INFO("%s: Init started", toString().c_str());
        _state.state = WheelStateEnum::INIT;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _initStartTime = millis();
        notifyStateChanged();

        const long maxSteps = (maxStepsPerRevolution > 0) ? maxStepsPerRevolution : _config.maxStepsPerRevolution;
        return move(maxSteps * _config.direction);
    }

    bool Wheel::moveToAngle(float angle)
    {
        if (_config.stepsPerRevolution <= 0)
        {
            MLOG_WARN("%s: Cannot move to $d° - steps per revolution not calibrated", toString().c_str(), angle);
            return false;
        }

        if (_state.lastZeroPosition == 0)
        {
            MLOG_WARN("%s: Cannot move to %d° - zero point not set", toString().c_str(), angle);
            return false;
        }

        // Normalize angle
        while (angle < 0)
            angle += 360;
        while (angle >= 360)
            angle -= 360;

        // Calculate target position
        long targetPosition = _state.lastZeroPosition + (angle / 360.0) * _config.stepsPerRevolution;
        long currentPosition = _stepper->getState().currentPosition;
        long stepsToMove = targetPosition - currentPosition;

        MLOG_INFO("%s: Moving to %.1f° (%ld -> %ld = %ld steps)",
                  toString().c_str(), angle, currentPosition, targetPosition, stepsToMove);

        return move(stepsToMove);
    }

    bool Wheel::nextBreakPoint()
    {
        if (_config.breakPoints.empty())
        {
            MLOG_WARN("%s: No breakpoints configured", toString().c_str());
            return false;
        }

        if (_state.lastZeroPosition == 0)
        {
            return init();
        }

        int nextIndex = (_state.currentBreakpointIndex + 1) % _config.breakPoints.size();
        _state.targetBreakpointIndex = nextIndex;

        // Calculate target angle
        float targetAngleRaw = _config.breakPoints[nextIndex];
        _state.targetAngle = targetAngleRaw; // Simplified

        MLOG_INFO("%s: Moving to next breakpoint index %d, angle %.1f°",
                  toString().c_str(), nextIndex, targetAngleRaw);
        return moveToAngle(_state.targetAngle);
    }

    bool Wheel::stop()
    {
        if (_state.state == WheelStateEnum::INIT || _state.state == WheelStateEnum::CALIBRATING)
        {
            _state.state = WheelStateEnum::IDLE;
            //  _state.targetBreakpointIndex = -1;
            //  _state.targetAngle = -1.0f;
            //  _waitingForMoveStart = false;
            //  _moveHasStarted = true;
            notifyStateChanged();
        }

        // Call stepper's stop method
        return _stepper->stop();
    }

    int Wheel::getCurrentBreakpointIndex() const
    {
        return _state.currentBreakpointIndex;
    }

    void Wheel::addStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["errorCode"] = static_cast<int>(_state.errorCode);
        doc["errorMessage"] = _state.errorMessage;
        doc["lastZeroPosition"] = _state.lastZeroPosition;
        doc["currentBreakpointIndex"] = _state.currentBreakpointIndex;
        doc["targetBreakpointIndex"] = _state.targetBreakpointIndex;
        doc["targetAngle"] = _state.targetAngle;
        doc["onError"] = _state.onError;
        doc["breakpointChanged"] = _state.breakpointChanged;
        doc["stepsInLastRevolution"] = _state.stepsInLastRevolution;
    }

    bool Wheel::control(const String &action, JsonObject *args)
    {
        if (action == "next-breakpoint")
        {
            return nextBreakPoint();
        }
        else if (action == "calibrate")
        {
            long maxSteps = -1;
            if (args && (*args)["maxStepsPerRevolution"].is<long>())
            {
                maxSteps = (*args)["maxStepsPerRevolution"].as<long>();
            }
            return calibrate(maxSteps);
        }
        else if (action == "init")
        {
            long maxSteps = -1;
            if (args && (*args)["maxStepsPerRevolution"].is<long>())
            {
                maxSteps = (*args)["maxStepsPerRevolution"].as<long>();
            }
            return init(maxSteps);
        }
        else if (action == "move-to-angle")
        {
            if (!args || !(*args)["angle"].is<float>())
                return false;
            float angle = (*args)["angle"].as<float>();
            _state.targetAngle = angle;
            return moveToAngle(angle);
        }
        else if (action == "stop")
        {
            return stop();
        }
        else
        {
            MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Wheel::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["stepsPerRevolution"].is<long>() || config["stepsPerRevolution"].is<int>() ||
            config["stepsPerRevolution"].is<float>() || config["stepsPerRevolution"].is<double>())
        {
            _config.stepsPerRevolution = config["stepsPerRevolution"].as<long>();
        }
        if (config["maxStepsPerRevolution"].is<long>() || config["maxStepsPerRevolution"].is<int>() ||
            config["maxStepsPerRevolution"].is<float>() || config["maxStepsPerRevolution"].is<double>())
        {
            _config.maxStepsPerRevolution = config["maxStepsPerRevolution"].as<long>();
        }
        if (config["zeroPointDegree"].is<float>() || config["zeroPointDegree"].is<double>() ||
            config["zeroPointDegree"].is<long>() || config["zeroPointDegree"].is<int>())
        {
            _config.zeroPointDegree = config["zeroPointDegree"].as<float>();
        }
        if (config["direction"].is<int>() || config["direction"].is<long>() ||
            config["direction"].is<float>() || config["direction"].is<double>())
        {
            _config.direction = config["direction"].as<int>();
        }

        // Load breakPoints - check if it exists and has a size (pragmatic approach to handle ArduinoJson type detection)
        if (config.containsKey("breakPoints") && config["breakPoints"].size() > 0)
        {
            _config.breakPoints.clear();
            size_t size = config["breakPoints"].size();
            for (size_t i = 0; i < size; i++)
            {
                if (config["breakPoints"][i].is<float>() || config["breakPoints"][i].is<double>() ||
                    config["breakPoints"][i].is<long>() || config["breakPoints"][i].is<int>())
                {
                    float bp = config["breakPoints"][i].as<float>();
                    _config.breakPoints.push_back(bp);
                }
                else if (config["breakPoints"][i].is<const char *>())
                {
                    const char *value = config["breakPoints"][i].as<const char *>();
                    if (value)
                    {
                        float bp = static_cast<float>(atof(value));
                        _config.breakPoints.push_back(bp);
                    }
                }
            }
        }
    }

    void Wheel::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["stepsPerRevolution"] = _config.stepsPerRevolution;
        doc["maxStepsPerRevolution"] = _config.maxStepsPerRevolution;
        doc["zeroPointDegree"] = _config.zeroPointDegree;
        doc["direction"] = _config.direction;

        JsonArray arr = doc["breakPoints"].to<JsonArray>();
        for (float bp : _config.breakPoints)
        {
            arr.add(bp);
        }
    }

    String Wheel::stateToString(WheelStateEnum state) const
    {
        switch (state)
        {
        case WheelStateEnum::UNKNOWN:
            return "UNKNOWN";
        case WheelStateEnum::CALIBRATING:
            return "CALIBRATING";
        case WheelStateEnum::IDLE:
            return "IDLE";
        case WheelStateEnum::MOVING:
            return "MOVING";
        case WheelStateEnum::INIT:
            return "INIT";
        case WheelStateEnum::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }

    void Wheel::setErrorState(WheelErrorCode errorCode, const String &errorMessage)
    {
        _state.state = WheelStateEnum::ERROR;
        _state.onError = true;
        _state.errorCode = errorCode;
        _state.errorMessage = errorMessage;
        MLOG_ERROR("%s: %s", toString().c_str(), errorMessage.c_str());
    }

    void Wheel::notifyStepsPerRevolution(long steps)
    {
        NotifyClients callback = ControllableMixinBase::getNotifyClients();
        if (!callback)
        {
            MLOG_INFO("%s: Measured steps per revolution: %ld", toString().c_str(), steps);
            return;
        }

        JsonDocument doc;
        doc["type"] = "steps-per-revolution";
        doc["deviceId"] = getId();
        doc["steps"] = steps;

        String message;
        serializeJson(doc, message);
        callback(message);
    }

} // namespace devices
