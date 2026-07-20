/**
 * @file Wheel.cpp
 * @brief Wheel implementation using Device and composition mixins
 */

#include "devices/Wheel.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include <cmath>
#include <cstdlib>

namespace devices
{

    // Default breakpoints (same as original)
    static const float defaultBreakpoints[] = {45.0, 90.0, 180.0, 270.0};

    Wheel::Wheel(const String &id)
        : Device(id, "wheel")
    {
        // See Device.h "Two-phase initialization contract".
        applyDefaultConfig();
    }

    Wheel::~Wheel()
    {
    }

    void Wheel::applyDefaultConfig()
    {
        _config.breakPoints.assign(std::begin(defaultBreakpoints), std::end(defaultBreakpoints));

        // Create stepper child
        _stepper = new Stepper(getId() + "-stepper");
        addChild(_stepper);

        // Create zero sensor button
        _zeroSensor = new Button(getId() + "-zero-sensor");
        addChild(_zeroSensor);

        // Set default config for stepper (overwritten by jsonToConfig in phase 2)
        JsonDocument stepperConfig;
        stepperConfig["name"] = "Wheel Stepper";
        stepperConfig["stepperType"] = "DRIVER";
        stepperConfig["maxSpeed"] = 3000;
        stepperConfig["maxAcceleration"] = 3000;
        stepperConfig["defaultSpeed"] = 1000;
        stepperConfig["defaultAcceleration"] = 200;

        _stepper->jsonToConfig(stepperConfig);

        // Set default config for zero sensor (overwritten by jsonToConfig in phase 2)
        JsonDocument sensorConfig;
        sensorConfig["name"] = "Wheel Zero Sensor";
        sensorConfig["pinMode"] = "pullup";
        sensorConfig["debounceMs"] = 50;
        sensorConfig["buttonType"] = "NormalOpen";
        _zeroSensor->jsonToConfig(sensorConfig);
    }

    void Wheel::setup()
    {
        Device::setup();
        setName(_config.name);

        if (!(_config.stepsPerRevolution > 0))
        {
            MLOG_ERROR("%s: Invalid stepsPerRevolution in config", toString().c_str());
            setErrorState(WheelErrorCode::ConfigError, "Invalid stepsPerRevolution in config");
            return;
        }

        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void Wheel::teardown()
    {
        Device::teardown();

        _state.state = WheelStateEnum::UNKNOWN;
        Device::clearError();
        _state.stepsInLastRevolution = 0;
        _state.lastZeroPosition = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _state.targetAngle = -1.0f;
        _state.currentAngle = -1.0f;
        _state.onError = false;
        _state.onBreakpointChanged = false;
        _waitingForMoveStart = false;
        _moveHasStarted = false;
    }

    void Wheel::loop()
    {
        Device::loop();

        // Always keep currentAngle up to date based on stepper position
        updateCurrentAngle();

        if (_state.onError)
        {
            _state.onError = false;
        }

        // Reset breakpoint changed flag
        if (_state.onBreakpointChanged)
        {
            _state.onBreakpointChanged = false;
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

            if (_waitingForMoveStart && stepperState.isMoving)
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
                    _state.onBreakpointChanged = true;
                }

                _state.state = WheelStateEnum::IDLE;
                updateCurrentAngle();
                _moveHasStarted = false;
                notifyStateChanged();
            }

            if (millis() % 100 == 0)
            {
                MLOG_DEBUG("%s: Moving... current position: %ld, target: %ld, speed: %.1f, accel: %.1f",
                           toString().c_str(), stepperState.currentPosition, stepperState.targetPosition,
                           stepperState.speed, stepperState.acceleration);
            }

            // Check zero sensor for position tracking
            if (_zeroSensor->onPressed())
            {

                // Zero sensor triggered mid-move: record how far we have come since the
                // last known zero so the update the targetPosition to compensate for the drift.
                long correction = stepperState.currentPosition - _config.stepsPerRevolution;

                if (correction != 0)
                {
                    MLOG_INFO("%s: Zero sensor triggered at %ld, expected at %ld, correction: %ld",
                              toString().c_str(), stepperState.currentPosition, _config.stepsPerRevolution, correction);

                    long newTargetPosition = stepperState.currentPosition - correction - _config.stepsPerRevolution;

                    MLOG_INFO("%s: Reset position and update target from %ld to %ld",
                              toString().c_str(), stepperState.targetPosition, newTargetPosition);

                    _stepper->setCurrentPosition(0);
                    _stepper->moveTo(newTargetPosition, stepperState.speed, stepperState.acceleration);
                }
            }
            else
            {
                // Detect missing zero sensor trigger during a revolution.
                const long stepsSinceZero = labs(stepperState.currentPosition - _state.lastZeroPosition);
                if (_config.maxStepsPerRevolution > 0 && stepsSinceZero >= (_config.maxStepsPerRevolution))
                {
                    setErrorState(WheelErrorCode::ZeroNotFound,
                                  "Zero sensor not triggered within maxStepsPerRevolution");
                    updateCurrentAngle();
                    notifyStateChanged();
                    break;
                }
            }
            break;
        }
        case WheelStateEnum::INIT:
        {

            // Check for zero sensor trigger
            if (_zeroSensor->onPressed())
            {
                // Zero sensor triggered while the motor is still moving.
                // Record the sensor position as the zero reference.
                long currentPosition = _stepper->getState().currentPosition;
                _state.lastZeroPosition = currentPosition;
                _stepper->setCurrentPosition(0);
                updateCurrentAngle();

                // Move to first breakpoint if configured.
                if (!_config.breakPoints.empty() && _config.stepsPerRevolution > 0)
                {
                    // Compute absolute stepper target for breakPoints[0].
                    long targetPosition = lroundf((_config.breakPoints[0] / 360.0f) * _config.stepsPerRevolution);

                    MLOG_INFO("%s: Init: Zero point reached, moving to first breakpoint %ld (%.1f°)...",
                              toString().c_str(), targetPosition, _config.breakPoints[0]);

                    _state.currentBreakpointIndex = -1;
                    _state.targetBreakpointIndex = 0;
                    _state.state = WheelStateEnum::MOVING;
                    _state.targetAngle = _config.breakPoints[0];
                    _waitingForMoveStart = true;
                    _moveHasStarted = false;
                    updateCurrentAngle();
                    notifyStateChanged();

                    _stepper->moveTo(targetPosition);
                }
                else
                {
                    MLOG_INFO("%s: Init: Zero point reached, no breakpoints configured", toString().c_str());
                    _stepper->stop();
                    _state.state = WheelStateEnum::IDLE;
                    updateCurrentAngle();
                    notifyStateChanged();
                }
            }

            // Stop moving during init
            else if ((millis() - _initStartTime > 20) && !_stepper->getState().isMoving)
            {
                // Movement completed without finding zero - error
                setErrorState(WheelErrorCode::CalibrationZeroNotFound, "Init: Zero sensor not found!");
                updateCurrentAngle();
                notifyStateChanged();
            }

            break;
        }
        case WheelStateEnum::CALIBRATING:
        {
            // Check for zero sensor trigger
            if (_zeroSensor->onPressed())
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
                    _state.lastZeroPosition = currentPosition;

                    // Update current angle after calibration
                    updateCurrentAngle();

                    // Stop the stepper
                    _stepper->stop();

                    // Set state to IDLE
                    _state.state = WheelStateEnum::IDLE;
                    notifyStateChanged();

                    notifyStepsPerRevolution(steps);

                    MLOG_INFO("%s: Calibration complete, steps per revolution: %ld", toString().c_str(), steps);
                    broadcastNotification(
                        "CalibrationComplete",
                        "Calibration complete: " + String(steps) + " steps per revolution",
                        DeviceNotificationType::Info);
                }
            }

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
                updateCurrentAngle();
                notifyStateChanged();
            }
            break;
        }
        default:
            break;
        }

        // Update current angle when position may have changed
        // if (_state.state == WheelStateEnum::MOVING || _state.state == WheelStateEnum::IDLE)
        // {
        //     updateCurrentAngle();
        // }
    }

    bool Wheel::move(long steps, float speedRatio)
    {
        // Update State to move (if not INIT or CALIBRATING)
        if (_state.state != WheelStateEnum::CALIBRATING && _state.state != WheelStateEnum::INIT)
        {
            // MLOG_INFO("%s: Moving %ld steps", toString().c_str(), steps);
            _state.state = WheelStateEnum::MOVING;
            _waitingForMoveStart = true;
            _moveHasStarted = false;
            updateCurrentAngle();
            notifyStateChanged();
        }

        // Call stepper's move method with optional speed scaling
        if (speedRatio <= 0.0f)
            speedRatio = 1.0f;
        const float speed = _stepper->getConfig().defaultSpeed * speedRatio;
        return _stepper->move(steps, speed);
    }

    bool Wheel::calibrate(long maxStepsPerRevolution)
    {
        if (!_stepper->isReady())
        {
            setErrorState(WheelErrorCode::StepperNotInitialized,
                          "Calibration failed: stepper is not initialized. Check stepper type and pin configuration.");
            notifyStateChanged();
            return false;
        }

        MLOG_INFO("%s: Calibration started", toString().c_str());
        Device::clearError();
        _state.state = WheelStateEnum::CALIBRATING;
        _state.lastZeroPosition = 0;
        _state.stepsInLastRevolution = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        updateCurrentAngle();
        notifyStateChanged();

        const long maxSteps = (maxStepsPerRevolution > 0) ? maxStepsPerRevolution : _config.maxStepsPerRevolution;
        // Move large number of steps to complete at least one revolution
        return move(maxSteps * 2 * _config.direction);
    }

    bool Wheel::init(long maxStepsPerRevolution, float speedRatio)
    {
        if (!_stepper->isReady())
        {
            setErrorState(WheelErrorCode::StepperNotInitialized,
                          "Init failed: stepper is not initialized. Check stepper type and pin configuration.");
            notifyStateChanged();
            return false;
        }

        MLOG_INFO("%s: Init started, looking for zero sensor", toString().c_str());
        Device::clearError();
        _state.state = WheelStateEnum::INIT;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _initStartTime = millis();
        updateCurrentAngle();
        notifyStateChanged();

        // Start moving to find zero position
        const long maxSteps = (maxStepsPerRevolution > 0) ? maxStepsPerRevolution : _config.maxStepsPerRevolution;
        return move(maxSteps * _config.direction, speedRatio);
    }

    bool Wheel::moveToAngle(float angle)
    {
        if (_config.stepsPerRevolution <= 0)
        {
            MLOG_WARN("%s: Cannot move to $d° - steps per revolution not calibrated", toString().c_str(), angle);
            return false;
        }

        // Normalize angle
        while (angle < 0)
            angle += 360;
        while (angle >= 360)
            angle -= 360;

        long currentPosition = _stepper->getState().currentPosition;

        // Always move forward: compute current wheel angle from zero reference,
        // then wrap target delta to [0, 360).
        // zeroPointDegree shifts the reference so the sensor position = zeroPointDegree°
        // and breakpoints are measured from physical zero.
        long zeroOffsetSteps = (_config.stepsPerRevolution > 0)
                                   ? lroundf((_config.zeroPointDegree / 360.0f) * _config.stepsPerRevolution)
                                   : 0L;
        long relSteps = currentPosition + zeroOffsetSteps;
        long relStepsNorm = relSteps % _config.stepsPerRevolution;
        if (relStepsNorm < 0)
            relStepsNorm += _config.stepsPerRevolution;
        float currentAngle = (relStepsNorm / (float)_config.stepsPerRevolution) * 360.0f;

        float angleDiff = angle - currentAngle;
        while (angleDiff < 0.0f)
            angleDiff += 360.0f;
        while (angleDiff >= 360.0f)
            angleDiff -= 360.0f;

        long stepsToMove = lroundf((angleDiff / 360.0f) * _config.stepsPerRevolution);
        long targetPosition = currentPosition + stepsToMove;

        if (stepsToMove == 0)
        {
            MLOG_INFO("%s: Already at %.1f°, no movement needed", toString().c_str(), angle);
            return true;
        }

        MLOG_INFO("%s: Moving to %.1f° (current %.1f°, forward diff %.1f° = %ld steps)",
                  toString().c_str(), angle, currentAngle, angleDiff, stepsToMove);

        return move(stepsToMove);
    }

    bool Wheel::nextBreakPoint(float speedRatio)
    {
        if (_config.breakPoints.empty())
        {
            MLOG_WARN("%s: No breakpoints configured", toString().c_str());
            return false;
        }

        int nextIndex = (_state.currentBreakpointIndex + 1) % _config.breakPoints.size(); // Rest on overflow
        _state.targetBreakpointIndex = nextIndex;

        // Calculate target angle
        float targetAngle = _config.breakPoints[nextIndex];

        // Update current angle before calculating movement
        updateCurrentAngle();

        // Use actual current angle instead of breakpoint angle (wheel may have drifted)
        float currentAngle = _state.currentAngle;

        // Always move forward to the next breakpoint.
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff < 0.0f)
            angleDiff += 360.0f;
        while (angleDiff >= 360.0f)
            angleDiff -= 360.0f;

        // Convert angle difference to steps
        long stepsToMove = lroundf((angleDiff / 360.0f) * _config.stepsPerRevolution);

        _state.targetAngle = targetAngle;

        // Special case: if there's only 1 breakpoint, rotate a full revolution instead of staying still
        if (targetAngle == currentAngle)
        {
            stepsToMove = _config.stepsPerRevolution;
            MLOG_INFO("%s: Only 1 breakpoint configured, rotating full revolution (%ld steps)",
                      toString().c_str(), stepsToMove);
        }

        MLOG_INFO("%s: Moving to next breakpoint index %d, angle %.1f° (from %.1f°, diff %.1f° = %ld steps)",
                  toString().c_str(), nextIndex, targetAngle, currentAngle, angleDiff, stepsToMove);
        return move(stepsToMove, speedRatio);
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
            updateCurrentAngle();
            notifyStateChanged();
        }

        // Call stepper's stop method
        return _stepper->stop();
    }

    int Wheel::getCurrentBreakpointIndex() const
    {
        return _state.currentBreakpointIndex;
    }

    void Wheel::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["lastZeroPosition"] = _state.lastZeroPosition;
        doc["currentBreakpointIndex"] = _state.currentBreakpointIndex;
        doc["targetBreakpointIndex"] = _state.targetBreakpointIndex;
        doc["targetAngle"] = _state.targetAngle;
        doc["currentAngle"] = _state.currentAngle;
        doc["onError"] = _state.onError;
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
        // ArduinoJson stores numbers with the originating type. When the
        // sender is JavaScript, integers arrive as int/long while floats
        // arrive as double. isNumeric() accepts all arithmetic variants so
        // we don't need four separate is<T>() checks per field.
        auto isNumeric = [](JsonVariantConst v)
        {
            return v.is<long>() || v.is<int>() || v.is<float>() || v.is<double>();
        };

        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (isNumeric(config["stepsPerRevolution"]))
            _config.stepsPerRevolution = config["stepsPerRevolution"].as<long>();
        if (isNumeric(config["maxStepsPerRevolution"]))
            _config.maxStepsPerRevolution = config["maxStepsPerRevolution"].as<long>();
        if (isNumeric(config["zeroPointDegree"]))
            _config.zeroPointDegree = config["zeroPointDegree"].as<float>();
        if (isNumeric(config["direction"]))
            _config.direction = config["direction"].as<int>();

        // Load breakPoints from JSON and coerce each item to float.
        JsonVariantConst breakPointsValue = config["breakPoints"];
        if (breakPointsValue.is<JsonArrayConst>())
        {
            JsonArrayConst breakPoints = breakPointsValue.as<JsonArrayConst>();
            _config.breakPoints.clear();
            for (JsonVariantConst item : breakPoints)
            {
                _config.breakPoints.push_back(item.as<float>());
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
        if (_stepper)
            _stepper->stop();
        Device::setError(errorCodeToString(errorCode), errorMessage);
    }

    String Wheel::errorCodeToString(WheelErrorCode errorCode) const
    {
        switch (errorCode)
        {
        case WheelErrorCode::None:
            return "None";
        case WheelErrorCode::CalibrationZeroNotFound:
            return "CalibrationZeroNotFound";
        case WheelErrorCode::CalibrationSecondZeroNotFound:
            return "CalibrationSecondZeroNotFound";
        case WheelErrorCode::ConfigError:
            return "CONFIG_ERROR";
        case WheelErrorCode::ZeroNotFound:
            return "ZeroNotFound";
        case WheelErrorCode::UnexpectedZeroTrigger:
            return "UnexpectedZeroTrigger";
        case WheelErrorCode::StepperNotInitialized:
            return "StepperNotInitialized";
        default:
            return "Unknown";
        }
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

    void Wheel::updateCurrentAngle()
    {
        if (_config.stepsPerRevolution > 0)
        {
            auto stepperState = _stepper->getState();
            float angle = (stepperState.currentPosition * 360.0f) / _config.stepsPerRevolution;

            // Normalize angle to 0-360 range
            while (angle < 0)
                angle += 360;
            while (angle >= 360)
                angle -= 360;
            _state.currentAngle = angle;
        }
        else
        {
            _state.currentAngle = -1.0f;
        }
    }

} // namespace devices
