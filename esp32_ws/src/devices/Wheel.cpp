/**
 * @file Wheel.cpp
 * @brief Wheel implementation using Device and composition mixins
 */

#include "devices/Wheel.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "Logging.h"
#include <ArduinoJson.h>

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

        // Create next button
        _nextButton = new Button(getId() + "-btn-next");
        addChild(_nextButton);
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
            // Check if next button is pressed
            if (_nextButton && _nextButton->hasMixin("state"))
            {
                // This would need to be implemented based on Button's state
                // For now, we'll skip this check
            }

            break;
        }
        case WheelStateEnum::MOVING:
        {
            // Check if movement completed
            // This would need stepper state integration
            // For now, assume movement is complete after some time

            // Check zero sensor
            if (_zeroSensor && _zeroSensor->hasMixin("state"))
            {
                // Handle zero sensor trigger
                long currentPosition = 0; // Would get from stepper
                _state.stepsInLastRevolution = currentPosition - _state.lastZeroPosition;
                _state.lastZeroPosition = currentPosition;

                // Check revolution consistency
                if (_config.stepsPerRevolution > 0)
                {
                    float percentDiff = abs(_state.stepsInLastRevolution - _config.stepsPerRevolution) /
                                        (float)_config.stepsPerRevolution * 100.0f;
                    if (percentDiff > 0.1f)
                    {
                        MLOG_ERROR("%s: Steps per revolution mismatch - measured: %ld, configured: %ld (%.2f%% difference)",
                                   toString().c_str(), _state.stepsInLastRevolution, _config.stepsPerRevolution, percentDiff);
                    }
                }
            }

            // Check if target breakpoint reached
            if (_state.targetBreakpointIndex >= 0)
            {
                _state.currentBreakpointIndex = _state.targetBreakpointIndex;
                _state.targetBreakpointIndex = -1;
                _state.breakpointChanged = true;
                _state.state = WheelStateEnum::IDLE;
                notifyStateChanged();
            }

            break;
        }
        case WheelStateEnum::INIT:
        {
            // Check for zero sensor during init
            if (_zeroSensor && _zeroSensor->hasMixin("state"))
            {
                long currentPosition = 0; // Would get from stepper
                _state.lastZeroPosition = currentPosition;

                // Move to first breakpoint
                if (!_config.breakPoints.empty() && _config.stepsPerRevolution > 0)
                {
                    MLOG_INFO("%s: Zero point found, moving to first breakpoint", toString().c_str());
                    _state.currentBreakpointIndex = -1;
                    _state.targetBreakpointIndex = 0;
                    _state.state = WheelStateEnum::MOVING;
                    _state.targetAngle = _config.breakPoints[0];
                    moveToAngle(_config.breakPoints[0]);
                    notifyStateChanged();
                }
                else
                {
                    MLOG_INFO("%s: Zero point found, no breakpoints yet", toString().c_str());
                    _state.state = WheelStateEnum::MOVING;
                    // Stop movement
                    stop();
                    notifyStateChanged();
                }
            }

            // Check if init failed (no zero found)
            // This would need timeout logic

            break;
        }
        case WheelStateEnum::CALIBRATING:
        {
            // Check for zero sensor during calibration
            if (_zeroSensor && _zeroSensor->hasMixin("state"))
            {
                if (_state.lastZeroPosition == 0)
                {
                    MLOG_INFO("%s: Calibration: zero found, counting steps per revolution", toString().c_str());
                    long currentPosition = 0; // Would get from stepper
                    _state.lastZeroPosition = currentPosition;
                }
                else
                {
                    long currentPos = 0; // Would get from stepper
                    _state.stepsInLastRevolution = currentPos - _state.lastZeroPosition;
                    MLOG_INFO("%s: Calibration complete, steps per revolution: %d", toString().c_str(), _state.stepsInLastRevolution);
                    _state.lastZeroPosition = currentPos;

                    notifyStepsPerRevolution(_state.stepsInLastRevolution);

                    _state.state = WheelStateEnum::MOVING;
                    notifyStateChanged();
                }
            }

            break;
        }
        default:
            break;
        }
    }

    std::vector<int> Wheel::getPins() const
    {
        // Collect pins from all children
        std::vector<int> allPins;
        for (auto child : getChildren())
        {
            auto childPins = child->getPins();
            allPins.insert(allPins.end(), childPins.begin(), childPins.end());
        }
        return allPins;
    }

    bool Wheel::move(long steps)
    {
        if (!_stepper)
        {
            MLOG_WARN("%s: Stepper not available", toString().c_str());
            return false;
        }

        // This would need to call stepper's move method
        // For now, return true
        MLOG_INFO("%s: Moving %ld steps", toString().c_str(), steps);
        return true;
    }

    bool Wheel::calibrate()
    {
        if (!_stepper)
        {
            MLOG_WARN("%s: Stepper not available for calibration", toString().c_str());
            return false;
        }

        MLOG_INFO("%s: Calibration started", toString().c_str());
        _state.state = WheelStateEnum::CALIBRATING;
        _state.lastZeroPosition = 0;
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;
        notifyStateChanged();

        // Move large number of steps
        return move(_config.maxStepsPerRevolution * 2 * _config.direction);
    }

    bool Wheel::init()
    {
        if (!_stepper)
        {
            MLOG_WARN("%s: Stepper not available for init", toString().c_str());
            return false;
        }

        MLOG_INFO("%s: Init started", toString().c_str());
        _state.state = WheelStateEnum::INIT;
        notifyStateChanged();
        _state.currentBreakpointIndex = -1;
        _state.targetBreakpointIndex = -1;

        return move(_config.maxStepsPerRevolution * _config.direction);
    }

    bool Wheel::moveToAngle(float angle)
    {
        if (!_stepper)
        {
            MLOG_WARN("%s: Stepper not available", toString().c_str());
            return false;
        }

        if (_config.stepsPerRevolution <= 0)
        {
            MLOG_WARN("%s: Cannot move to angle - steps per revolution not calibrated", toString().c_str());
            return false;
        }

        if (_state.lastZeroPosition == 0)
        {
            MLOG_WARN("%s: Cannot move to angle - zero point not set", toString().c_str());
            return false;
        }

        // Normalize angle
        while (angle < 0)
            angle += 360;
        while (angle >= 360)
            angle -= 360;

        // Calculate target position
        long targetPosition = _state.lastZeroPosition + (angle / 360.0) * _config.stepsPerRevolution;
        long currentPosition = 0; // Would get from stepper
        long stepsToMove = targetPosition - currentPosition;

        MLOG_INFO("%s: Moving to absolute angle %.1f° (target pos: %ld, current pos: %ld, steps: %ld)",
                  toString().c_str(), angle, targetPosition, currentPosition, stepsToMove);

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
        if (!_stepper)
        {
            return false;
        }

        // This would call stepper's stop method
        MLOG_INFO("%s: Stop requested", toString().c_str());
        return true;
    }

    int Wheel::getCurrentBreakpointIndex() const
    {
        return _state.currentBreakpointIndex;
    }

    void Wheel::addStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["lastZeroPosition"] = _state.lastZeroPosition;
        doc["currentBreakpointIndex"] = _state.currentBreakpointIndex;
        doc["targetBreakpointIndex"] = _state.targetBreakpointIndex;
        doc["targetAngle"] = _state.targetAngle;
        doc["onError"] = _state.onError;
        doc["breakpointChanged"] = _state.breakpointChanged;
    }

    bool Wheel::control(const String &action, JsonObject *args)
    {
        if (action == "next-breakpoint")
        {
            return nextBreakPoint();
        }
        else if (action == "calibrate")
        {
            return calibrate();
        }
        else if (action == "init")
        {
            return init();
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
        if (config["stepsPerRevolution"].is<long>())
            _config.stepsPerRevolution = config["stepsPerRevolution"].as<long>();
        if (config["maxStepsPerRevolution"].is<long>())
            _config.maxStepsPerRevolution = config["maxStepsPerRevolution"].as<long>();
        if (config["zeroPointDegree"].is<float>())
            _config.zeroPointDegree = config["zeroPointDegree"].as<float>();
        if (config["direction"].is<int>())
            _config.direction = config["direction"].as<int>();

        if (config["breakPoints"].is<JsonArray>())
        {
            _config.breakPoints.clear();
            size_t size = config["breakPoints"].size();
            for (size_t i = 0; i < size; i++)
            {
                if (config["breakPoints"][i].is<float>() || config["breakPoints"][i].is<long>())
                {
                    _config.breakPoints.push_back(config["breakPoints"][i].as<float>());
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

    void Wheel::plotState()
    {
        // Convert state enum to numeric: 0=UNKNOWN, 1=CALIBRATING, 2=IDLE, 3=MOVING, 4=INIT, 5=ERROR
        int stateNumeric = static_cast<int>(_state.state);
        
        MLOG_PLOT("%s_state:%d,%s_lastZeroPosition:%ld,%s_stepsInLastRevolution:%ld,%s_currentBreakpointIndex:%d,%s_targetBreakpointIndex:%d,%s_targetAngle:%.1f,%s_onError:%d,%s_breakpointChanged:%d", 
                  _id.c_str(), stateNumeric,
                  _id.c_str(), _state.lastZeroPosition,
                  _id.c_str(), _state.stepsInLastRevolution,
                  _id.c_str(), _state.currentBreakpointIndex,
                  _id.c_str(), _state.targetBreakpointIndex,
                  _id.c_str(), _state.targetAngle,
                  _id.c_str(), _state.onError ? 1 : 0,
                  _id.c_str(), _state.breakpointChanged ? 1 : 0);
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

    void Wheel::notifyStepsPerRevolution(long steps)
    {
        // This would notify via WebSocket if needed
        MLOG_INFO("%s: Measured steps per revolution: %ld", toString().c_str(), steps);
    }

    Device *Wheel::getStepper() const
    {
        return _stepper;
    }

    Device *Wheel::getZeroSensor() const
    {
        return _zeroSensor;
    }

    Device *Wheel::getNextButton() const
    {
        return _nextButton;
    }

} // namespace devices
