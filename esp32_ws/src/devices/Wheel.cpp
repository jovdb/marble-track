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

        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void Wheel::teardown()
    {
        Device::teardown();

        _state.state = WheelStateEnum::UNKNOWN;
        Device::clearError();
        _state.lastZeroPosition = 0;
        _state.pendingZeroOffset = 0;
        _state.stepsInLastRevolution = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _state.targetAngle = -1.0f;
        _state.currentAngle = -1.0f;
        _state.onError = false;
        _state.breakpointChanged = false;
        _state.zeroSensorWasPressed = false;
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

            if (_waitingForMoveStart && stepperState.isMoving)
            {
                _waitingForMoveStart = false;
                _moveHasStarted = true;
            }

            // Check if movement completed (only after movement has actually started)
            if (!_waitingForMoveStart && _moveHasStarted && !stepperState.isMoving)
            {
                MLOG_INFO("%s: Movement to target completed", toString().c_str());

                // Apply pending zero offset if any
                if (_state.pendingZeroOffset != 0)
                {
                    _state.stepsInLastRevolution = _state.pendingZeroOffset;
                    if (_driftCorrectionApplied)
                    {
                        // moveTo correction already placed the motor at the exact target.
                        // Only update lastZeroPosition — do NOT reset the stepper counter;
                        // the internal position IS the correct absolute position.
                        _state.lastZeroPosition += _state.pendingZeroOffset;
                        _driftCorrectionApplied = false;
                    }
                    else
                    {
                        // Relative move: roll the internal counter back by one revolution
                        // so the counter stays manageable and angle arithmetic stays in range.
                        long currentPosition = _stepper->getState().currentPosition;
                        _stepper->setCurrentPosition(currentPosition - _state.pendingZeroOffset);
                        _state.lastZeroPosition += _state.pendingZeroOffset;
                    }

                    // Check revolution consistency (warn only; don't error on small drift)
                    if (_config.stepsPerRevolution > 0 && _state.stepsInLastRevolution > 0)
                    {
                        float percentDiff = std::abs(_state.stepsInLastRevolution - _config.stepsPerRevolution) / (float)_config.stepsPerRevolution * 100.0f;
                        if (percentDiff > 5.0f)
                        {
                            char errorMessage[128];
                            snprintf(errorMessage, sizeof(errorMessage), "Steps per revolution mismatch - measured: %ld, configured: %ld (%.2f%% difference)", _state.stepsInLastRevolution, _config.stepsPerRevolution, percentDiff);
                            setErrorState(WheelErrorCode::UnexpectedZeroTrigger, errorMessage);
                        }
                        else if (percentDiff > 0.1f)
                        {
                            MLOG_WARN("%s: Minor revolution drift: measured %ld, configured %ld (%.2f%%)",
                                      toString().c_str(), _state.stepsInLastRevolution, _config.stepsPerRevolution, percentDiff);
                        }
                    }

                    _state.pendingZeroOffset = 0;
                }

                if (_state.targetBreakpointIndex >= 0)
                {
                    _state.currentBreakpointIndex = _state.targetBreakpointIndex;
                    _state.targetBreakpointIndex = -1;
                    _state.breakpointChanged = true;
                }

                _state.state = WheelStateEnum::IDLE;
                updateCurrentAngle();
                _moveHasStarted = false;
                notifyStateChanged();
            }

            // Check zero sensor for position tracking
            bool zeroPressed = _zeroSensor->getState().isPressed;
            if (zeroPressed && !_state.zeroSensorWasPressed)
            {
                // Zero sensor triggered mid-move: record how far we have come since the
                // last known zero so the post-move block can update the position reference.
                long currentPosition = _stepper->getState().currentPosition;
                _state.pendingZeroOffset = currentPosition - _state.lastZeroPosition;

                // Real-time drift correction: recompute the absolute target from the
                // sensor's exact position, mirroring what the INIT state already does.
                // This corrects the current move, not just the next one.
                if (_state.targetAngle >= 0.0f && _config.stepsPerRevolution > 0)
                {
                    long zeroOffsetSteps = lroundf((_config.zeroPointDegree / 360.0f) * _config.stepsPerRevolution);
                    long stepsToTarget   = lroundf((_state.targetAngle            / 360.0f) * _config.stepsPerRevolution);
                    long absoluteTarget  = currentPosition - zeroOffsetSteps + stepsToTarget;
                    // Always move forward
                    if (absoluteTarget <= currentPosition)
                        absoluteTarget += _config.stepsPerRevolution;

                    MLOG_INFO("%s: Zero triggered mid-move at %ld, correcting target to %ld (%.1f°)",
                              toString().c_str(), currentPosition, absoluteTarget, _state.targetAngle);
                    _stepper->moveTo(absoluteTarget);
                    _driftCorrectionApplied = true;
                }
            }
            else
            {
                const long currentPosition = _stepper->getState().currentPosition;
                const long effectiveLastZero = _state.lastZeroPosition + _state.pendingZeroOffset;
                const long stepsSinceZero = labs(currentPosition - effectiveLastZero);
                if (_config.maxStepsPerRevolution > 0 && stepsSinceZero >= _config.maxStepsPerRevolution)
                {
                    setErrorState(WheelErrorCode::ZeroNotFound,
                                  "Zero sensor not triggered within maxStepsPerRevolution");
                    updateCurrentAngle();
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
                // Zero sensor triggered while the motor is still moving.
                // Record the sensor position as the zero reference.
                long currentPosition = _stepper->getState().currentPosition;
                _state.lastZeroPosition = currentPosition;
                updateCurrentAngle();

                // Move to first breakpoint if configured.
                // Use moveTo() (absolute target) instead of stop() + moveToAngle() (relative).
                // stop() puts AccelStepper in deceleration mode (_n < 0); an immediate
                // move() call then causes the speed profile to overshoot by ~22 steps.
                // moveTo() issues one clean absolute target so AccelStepper decelerates
                // smoothly from its current speed and stops exactly at the right position.
                if (!_config.breakPoints.empty() && _config.stepsPerRevolution > 0)
                {
                    // Compute absolute stepper target for breakPoints[0].
                    // zeroPointDegree: sensor fires this many degrees past physical zero,
                    // so subtract that offset so breakpoints are measured from physical zero.
                    long zeroOffsetSteps   = lroundf((_config.zeroPointDegree / 360.0f) * _config.stepsPerRevolution);
                    long stepsToBreakpoint = lroundf((_config.breakPoints[0]  / 360.0f) * _config.stepsPerRevolution);
                    long absoluteTarget    = currentPosition - zeroOffsetSteps + stepsToBreakpoint;
                    // Always move forward; if the target falls behind current position
                    // (e.g. breakPoints[0] < zeroPointDegree), advance one full revolution.
                    if (absoluteTarget <= currentPosition)
                        absoluteTarget += _config.stepsPerRevolution;

                    MLOG_INFO("%s: Init: Zero point reached at %ld, moving to first breakpoint (abs target %ld)...",
                              toString().c_str(), currentPosition, absoluteTarget);

                    _state.currentBreakpointIndex = -1;
                    _state.targetBreakpointIndex  = 0;
                    _state.state                  = WheelStateEnum::MOVING;
                    _state.targetAngle            = _config.breakPoints[0];
                    _waitingForMoveStart          = true;
                    _moveHasStarted               = false;
                    _state.pendingZeroOffset      = 0;
                    updateCurrentAngle();
                    notifyStateChanged();

                    _stepper->moveTo(absoluteTarget);
                }
                else
                {
                    MLOG_INFO("%s: Init: Zero point reached at %ld, no breakpoints configured", toString().c_str(), currentPosition);
                    _stepper->stop();
                    _state.state = WheelStateEnum::IDLE;
                    updateCurrentAngle();
                    notifyStateChanged();
                }
            }
            else if ((millis() - _initStartTime > 300) && !_stepper->getState().isMoving)
            {
                // Movement completed without finding zero - error
                setErrorState(WheelErrorCode::CalibrationZeroNotFound, "Init: Zero sensor not found!");
                updateCurrentAngle();
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

                    // Update current angle after calibration
                    updateCurrentAngle();

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
        if (_state.state != WheelStateEnum::CALIBRATING && _state.state != WheelStateEnum::INIT)
        {
            // MLOG_INFO("%s: Moving %ld steps", toString().c_str(), steps);
            _state.state = WheelStateEnum::MOVING;
            _waitingForMoveStart = true;
            _moveHasStarted = false;
            _state.pendingZeroOffset = 0;
            _driftCorrectionApplied = false;
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
        _state.pendingZeroOffset = 0;
        _state.stepsInLastRevolution = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _state.zeroSensorWasPressed = _zeroSensor->getState().isPressed; // Initialize to current state
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

        MLOG_INFO("%s: Init started", toString().c_str());
        Device::clearError();
        _state.state = WheelStateEnum::INIT;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        _initStartTime = millis();
        updateCurrentAngle();
        notifyStateChanged();

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

        long currentPosition = _stepper->getState().currentPosition;

        // Always move forward: compute current wheel angle from zero reference,
        // then wrap target delta to [0, 360).
        // zeroPointDegree shifts the reference so the sensor position = zeroPointDegree°
        // and breakpoints are measured from physical zero.
        long zeroOffsetSteps = (_config.stepsPerRevolution > 0)
            ? lroundf((_config.zeroPointDegree / 360.0f) * _config.stepsPerRevolution)
            : 0L;
        float currentAngle = 0.0f;
        long relSteps = currentPosition - _state.lastZeroPosition + zeroOffsetSteps;
        long relStepsNorm = relSteps % _config.stepsPerRevolution;
        if (relStepsNorm < 0)
            relStepsNorm += _config.stepsPerRevolution;
        currentAngle = (relStepsNorm / (float)_config.stepsPerRevolution) * 360.0f;

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

        if (_state.lastZeroPosition == 0)
        {
            return init();
        }

        int nextIndex = (_state.currentBreakpointIndex + 1) % _config.breakPoints.size();
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
        doc["pendingZeroOffset"] = _state.pendingZeroOffset;
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
        auto isNumeric = [](JsonVariantConst v) {
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
        if (_state.lastZeroPosition != 0 && _config.stepsPerRevolution > 0)
        {
            long currentPosition = _stepper->getState().currentPosition;
            long effectiveLastZero = _state.lastZeroPosition + _state.pendingZeroOffset;
            // zeroPointDegree: the sensor fires at this many degrees past physical zero.
            // Adding the corresponding steps makes "at sensor" = zeroPointDegree°.
            long zeroOffsetSteps = lroundf((_config.zeroPointDegree / 360.0f) * _config.stepsPerRevolution);
            long stepsFromZero = currentPosition - effectiveLastZero + zeroOffsetSteps;
            float angle = (stepsFromZero * 360.0f) / _config.stepsPerRevolution;
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
