#include "devices/Lift.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "devices/Servo.h"
#include "Logging.h"

namespace devices
{

    /* Move 2% extra down */
    const float DOWN_FACTOR = 1.015f; // Move 2% extra when going down to ensure full descent

    Lift::Lift(const String &id)
        : DeviceBase(id, "lift")
    {
        _stepper = nullptr;
        _limitSwitch = nullptr;
        _ballSensor = nullptr;
        _loader = nullptr;
        _unloader = nullptr;

        _stateMutex = xSemaphoreCreateMutex();

        // Create children
        _stepper = new Stepper(getId() + "-stepper");
        _limitSwitch = new Button(getId() + "-limit");
        _ballSensor = new Button(getId() + "-ball-sensor");
        _loader = new Servo(getId() + "-loader");
        _unloader = new Servo(getId() + "-unloader");
        addChild(_stepper);
        addChild(_limitSwitch);
        addChild(_ballSensor);
        addChild(_loader);
        addChild(_unloader);
    }

    Lift::~Lift()
    {
        if (_stateMutex)
        {
            vSemaphoreDelete(_stateMutex);
        }
    }

    void Lift::setup()
    {
        DeviceBase::setup();

        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void Lift::loop()
    {
        DeviceBase::loop();

        if (!_stepper)
            return;

        // Check ball sensor state and notify if changed
        bool ballWaiting = false;
        if (_ballSensor)
        {
            auto ballSensor = getBallSensor();
            if (ballSensor && ballSensor->hasMixin("state"))
            {
                // This is a simplified check - in real implementation would need to access state
                ballWaiting = true; // Placeholder
            }
        }

        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {
            bool changed = (_state.isBallWaiting != ballWaiting);
            _state.isBallWaiting = ballWaiting;

            if (changed)
            {
                MLOG_INFO("Lift [%s]: Ball waiting state changed to %s", getId().c_str(), ballWaiting ? "true" : "false");
                notifyStateChanged();
            }

            xSemaphoreGive(_stateMutex);
        }

        // State machine logic
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {
            switch (_state.state)
            {
            case LiftStateEnum::UNKNOWN:
            {
                auto limitSwitch = getLimitSwitch();
                if (limitSwitch && limitSwitch->hasMixin("state"))
                {
                    // Check if at limit - simplified check
                    auto stepper = getStepper();
                    if (stepper && stepper->hasMixin("state"))
                    {
                        // Set current position to 0 if at limit
                        _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
                        notifyStateChanged();
                    }
                }
                break;
            }
            case LiftStateEnum::INIT:
                // Handle reset sequence steps
                handleInitSequence();
                break;
            case LiftStateEnum::ERROR:
                // In error state, do nothing - requires manual reset
                break;
            case LiftStateEnum::LIFT_DOWN_LOADING:
                // Wait 1 second after starting load, then end the loading process
                if (millis() - _state.loadStartTime >= 1000)
                {
                    loadBallEnd();
                }
                break;
            case LiftStateEnum::LIFT_DOWN_LOADED:
                break;
            case LiftStateEnum::LIFT_UP_UNLOADING:
                // Wait 2 seconds after starting unload, then end the unloading process
                if (millis() - _state.unloadStartTime >= 2000)
                {
                    unloadBallEnd();
                }
                break;
            case LiftStateEnum::LIFT_UP_UNLOADED:
                break;
            case LiftStateEnum::LIFT_UP_LOADED:
                break;
            case LiftStateEnum::MOVING_UP:
                if (isStepperIdle())
                {
                    MLOG_INFO("Lift [%s]: Top reached", getId().c_str());
                    _state.state = _state.isLoaded ? LiftStateEnum::LIFT_UP_LOADED : LiftStateEnum::LIFT_UP_UNLOADED;
                    notifyStateChanged();
                }
                break;
            case LiftStateEnum::MOVING_DOWN:
                // When stepper stops moving, return to appropriate idle state
                if (isStepperIdle())
                {
                    MLOG_INFO("Lift [%s]: Movement complete", getId().c_str());
                    _state.state = _state.isLoaded ? LiftStateEnum::LIFT_DOWN_LOADED : LiftStateEnum::LIFT_DOWN_UNLOADED;
                    notifyStateChanged();
                }
                else if (isAtLimit() && _state.state == LiftStateEnum::MOVING_DOWN)
                {
                    MLOG_WARN("Lift [%s]: Limit switch triggered during downward movement - stopping", getId().c_str());
                    stopStepper();
                    _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
                    notifyStateChanged();
                }
                break;
            default:
                setError("LIFT_UNKNOWN_STATE_LOOP", "Unknown state encountered in loop()");
            }

            xSemaphoreGive(_stateMutex);
        }
    }

    std::vector<int> Lift::getPins() const
    {
        std::vector<int> pins;
        // Collect pins from all children
        for (auto child : getChildren())
        {
            auto childPins = child->getPins();
            pins.insert(pins.end(), childPins.begin(), childPins.end());
        }
        return pins;
    }

    bool Lift::up(float speedRatio)
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE)
            return false;

        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("Lift [%s]: Cannot move up, state is %s", getId().c_str(), stateToString(_state.state).c_str());
            xSemaphoreGive(_stateMutex);
            return false;

        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        case LiftStateEnum::LIFT_DOWN_LOADED:
        case LiftStateEnum::MOVING_DOWN:
        case LiftStateEnum::MOVING_UP: // for changed speed
        {
            // Check if lift is already at or above max position
            long currentPos = getCurrentPosition();
            if (currentPos >= _config.maxSteps)
            {
                MLOG_WARN("Lift [%s]: Cannot move up - already at max position (current: %ld, max: %ld)", getId().c_str(), currentPos, _config.maxSteps);
                xSemaphoreGive(_stateMutex);
                return false;
            }

            MLOG_INFO("Lift [%s]: Moving up to %ld steps", getId().c_str(), _config.maxSteps);
            _state.state = LiftStateEnum::MOVING_UP;
            notifyStateChanged();
            xSemaphoreGive(_stateMutex);
            return moveStepperTo(_config.maxSteps, speedRatio);
        }
        default:
            setError("LIFT_UNKNOWN_STATE_UP", "Unknown state encountered in up()");
            xSemaphoreGive(_stateMutex);
            return false;
        }
    }

    bool Lift::down(float speedRatio)
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE)
            return false;

        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        case LiftStateEnum::LIFT_DOWN_LOADED:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("Lift [%s]: Cannot move down, state is %s", getId().c_str(), stateToString(_state.state).c_str());
            xSemaphoreGive(_stateMutex);
            return false;

        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        case LiftStateEnum::MOVING_DOWN: // for changed speed
        case LiftStateEnum::MOVING_UP:
        {
            // Check if lift is already at or below min position
            long currentPos = getCurrentPosition();
            if (currentPos <= _config.minSteps)
            {
                MLOG_WARN("Lift [%s]: Cannot move down - already at min position (current: %ld, min: %ld)", getId().c_str(), currentPos, _config.minSteps);
                xSemaphoreGive(_stateMutex);
                return false;
            }

            long steps = (_config.minSteps - currentPos) * DOWN_FACTOR;
            _state.state = LiftStateEnum::MOVING_DOWN;
            notifyStateChanged();
            xSemaphoreGive(_stateMutex);
            return moveStepper(steps, speedRatio);
        }
        default:
            setError("LIFT_UNKNOWN_STATE_DOWN", "Unknown state encountered in down()");
            xSemaphoreGive(_stateMutex);
            return false;
        }
    }

    bool Lift::init()
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE)
            return false;

        if (!_stepper || !_limitSwitch)
        {
            MLOG_WARN("Lift [%s]: Stepper or limit switch not initialized", getId().c_str());
            xSemaphoreGive(_stateMutex);
            return false;
        }

        MLOG_INFO("Lift [%s]: Starting init sequence", getId().c_str());

        _state.state = LiftStateEnum::INIT;
        _state.initStep = 1; // unload end

        notifyStateChanged();
        xSemaphoreGive(_stateMutex);
        return true;
    }

    bool Lift::loadBall()
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE)
            return false;

        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::MOVING_UP:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        case LiftStateEnum::LIFT_UP_UNLOADING:
        case LiftStateEnum::MOVING_DOWN:
        case LiftStateEnum::LIFT_DOWN_LOADED:
            MLOG_WARN("Lift [%s]: Cannot load ball, state is %s", getId().c_str(), stateToString(_state.state).c_str());
            xSemaphoreGive(_stateMutex);
            return false;

        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        {
            bool result = loadBallStart();
            xSemaphoreGive(_stateMutex);
            return result;
        }
        default:
            setError("LIFT_UNKNOWN_STATE_LOAD_BALL", "Unknown state encountered in loadBall()");
            xSemaphoreGive(_stateMutex);
            return false;
        }
    }

    bool Lift::unloadBall()
    {
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE)
            return false;

        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::MOVING_UP:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADING:
        case LiftStateEnum::MOVING_DOWN:
        case LiftStateEnum::LIFT_DOWN_LOADED:
        case LiftStateEnum::LIFT_DOWN_UNLOADED:
            MLOG_WARN("Lift [%s]: Cannot unload ball, state is %s", getId().c_str(), stateToString(_state.state).c_str());
            xSemaphoreGive(_stateMutex);
            return false;

        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        {
            bool result = unloadBallStart();
            xSemaphoreGive(_stateMutex);
            return result;
        }
        default:
            setError("LIFT_UNKNOWN_STATE_UNLOAD_BALL", "Unknown state encountered in unloadBall()");
            xSemaphoreGive(_stateMutex);
            return false;
        }
    }

    bool Lift::isBallWaiting() const
    {
        return _state.isBallWaiting;
    }

    bool Lift::isLoaded() const
    {
        return _state.isLoaded;
    }

    void Lift::addStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["isBallWaiting"] = _state.isBallWaiting;
        doc["isLoaded"] = _state.isLoaded;
        doc["currentPosition"] = getCurrentPosition();
    }

    bool Lift::control(const String &action, JsonObject *args)
    {
        if (action == "up")
        {
            float speedRatio = 1.0f;
            if (args && (*args)["speedRatio"].is<float>())
            {
                speedRatio = (*args)["speedRatio"];
            }
            return up(speedRatio);
        }
        else if (action == "down")
        {
            float speedRatio = 1.0f;
            if (args && (*args)["speedRatio"].is<float>())
            {
                speedRatio = (*args)["speedRatio"];
            }
            return down(speedRatio);
        }
        else if (action == "init")
        {
            return init();
        }
        else if (action == "loadBall")
        {
            return loadBall();
        }
        else if (action == "unloadBall")
        {
            return unloadBall();
        }
        else
        {
            MLOG_WARN("Lift [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
        }

        return false;
    }

    void Lift::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
        if (config["minSteps"].is<long>())
        {
            _config.minSteps = config["minSteps"];
        }
        if (config["maxSteps"].is<long>())
        {
            _config.maxSteps = config["maxSteps"];
        }
        if (config["downFactor"].is<float>())
        {
            _config.downFactor = config["downFactor"];
        }
    }

    void Lift::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["minSteps"] = _config.minSteps;
        doc["maxSteps"] = _config.maxSteps;
        doc["downFactor"] = _config.downFactor;
    }

    String Lift::stateToString(LiftStateEnum state) const
    {
        switch (state)
        {
        case LiftStateEnum::UNKNOWN:
            return "Unknown";
        case LiftStateEnum::ERROR:
            return "Error";
        case LiftStateEnum::INIT:
            return "Init";
        case LiftStateEnum::LIFT_DOWN_LOADING:
            return "LiftDownLoading";
        case LiftStateEnum::LIFT_DOWN_LOADED:
            return "LiftDownLoaded";
        case LiftStateEnum::LIFT_UP_UNLOADING:
            return "LiftUpUnloading";
        case LiftStateEnum::LIFT_UP_UNLOADED:
            return "LiftUpUnloaded";
        case LiftStateEnum::LIFT_UP_LOADED:
            return "LiftUpLoaded";
        case LiftStateEnum::LIFT_DOWN_UNLOADED:
            return "LiftDownUnloaded";
        case LiftStateEnum::MOVING_UP:
            return "MovingUp";
        case LiftStateEnum::MOVING_DOWN:
            return "MovingDown";
        default:
            return "Unknown";
        }
    }

    bool Lift::loadBallStart()
    {
        auto loader = getLoader();
        if (!loader)
        {
            MLOG_WARN("Lift [%s]: Lift Loader not initialized", getId().c_str());
            return false;
        }

        MLOG_INFO("Lift [%s]: Loading ball...", getId().c_str());
        _state.state = LiftStateEnum::LIFT_DOWN_LOADING;
        _state.loadStartTime = millis();
        _state.isLoaded = true;
        notifyStateChanged();

        // Set loader to 100 (fully open) - simplified control
        return true; // Placeholder
    }

    bool Lift::loadBallEnd()
    {
        auto loader = getLoader();
        if (!loader)
        {
            MLOG_WARN("Lift [%s]: Lift Loader not initialized", getId().c_str());
            return false;
        }

        _state.state = LiftStateEnum::LIFT_DOWN_LOADED;
        notifyStateChanged();

        // Set loader to 0 (fully closed) - simplified control
        return true; // Placeholder
    }

    bool Lift::unloadBallStart()
    {
        auto unloader = getUnloader();
        if (!unloader)
        {
            MLOG_WARN("Lift [%s]: Lift Unloader not initialized", getId().c_str());
            return false;
        }

        MLOG_INFO("Lift [%s]: Unloading ball...", getId().c_str());
        _state.state = LiftStateEnum::LIFT_UP_UNLOADING;
        _state.unloadStartTime = millis();
        _state.isLoaded = false;
        notifyStateChanged();

        // Set unloader to 100 (fully open) - simplified control
        return true; // Placeholder
    }

    bool Lift::unloadBallEnd()
    {
        auto unloader = getUnloader();
        if (!unloader)
        {
            MLOG_WARN("Lift [%s]: Lift Unloader not initialized", getId().c_str());
            return false;
        }

        _state.state = LiftStateEnum::LIFT_UP_UNLOADED;
        notifyStateChanged();

        // Set unloader to 0 (fully closed) - simplified control
        return true; // Placeholder
    }

    DeviceBase *Lift::getStepper() const
    {
        return _stepper;
    }

    DeviceBase *Lift::getLimitSwitch() const
    {
        return _limitSwitch;
    }

    DeviceBase *Lift::getBallSensor() const
    {
        return _ballSensor;
    }

    DeviceBase *Lift::getLoader() const
    {
        return _loader;
    }

    DeviceBase *Lift::getUnloader() const
    {
        return _unloader;
    }

    // Helper methods for stepper control - simplified implementations
    long Lift::getCurrentPosition() const
    {
        // Placeholder - would need to access stepper state
        return 0;
    }

    bool Lift::isStepperIdle() const
    {
        // Placeholder - would need to access stepper state
        return true;
    }

    bool Lift::isAtLimit() const
    {
        // Placeholder - would need to access limit switch state
        return false;
    }

    bool Lift::moveStepper(long steps, float speedRatio)
    {
        // Placeholder - would need to control stepper
        return true;
    }

    bool Lift::moveStepperTo(long position, float speedRatio)
    {
        // Placeholder - would need to control stepper
        return true;
    }

    bool Lift::stopStepper()
    {
        // Placeholder - would need to control stepper
        return true;
    }

    void Lift::setError(const String &errorCode, const String &message)
    {
        MLOG_ERROR("Lift [%s]: %s - %s", getId().c_str(), errorCode.c_str(), message.c_str());
        _state.state = LiftStateEnum::ERROR;
        _state.onError = true;
        notifyStateChanged();
    }

    void Lift::handleInitSequence()
    {
        // Simplified init sequence - would need full implementation
        switch (_state.initStep)
        {
        case 1:
            // Move unload out of the way
            _state.initStep = 2;
            break;
        // ... more steps would be implemented
        default:
            _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
            _state.initStep = 0;
            notifyStateChanged();
            break;
        }
    }

} // namespace devices
