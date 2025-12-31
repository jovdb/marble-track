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
        : Device(id, "lift")
    {
        // Set default lift configuration
        _config.name = "Lift";
        _config.minSteps = 0;
        _config.maxSteps = 2255;

        // Create children with default configurations
        _stepper = new Stepper(getId() + "-stepper");
        auto stepperCfg = _stepper->getConfig();
        stepperCfg.name = "Lift Stepper";
        stepperCfg.stepperType = "DRIVER";
        stepperCfg.maxSpeed = 400.0f;
        stepperCfg.maxAcceleration = 100.0f;
        stepperCfg.defaultSpeed = 150.0f;
        stepperCfg.defaultAcceleration = 50.0f;
        stepperCfg.stepPin = 1;
        stepperCfg.dirPin = 2;
        stepperCfg.enablePin = 42;
        stepperCfg.invertEnable = true;
        _stepper->setConfig(stepperCfg);
        addChild(_stepper);

        _limitSwitch = new Button(getId() + "-limit");
        auto limitSwitchCfg = _limitSwitch->getConfig();
        limitSwitchCfg.name = "Lift Limit Switch";
        limitSwitchCfg.pin = 41;
        limitSwitchCfg.pinMode = PinModeOption::PullUp;
        limitSwitchCfg.buttonType = ButtonType::NormalOpen;
        _limitSwitch->setConfig(limitSwitchCfg);
        addChild(_limitSwitch);

        _ballSensor = new Button(getId() + "-ball-sensor");
        auto ballSensorCfg = _ballSensor->getConfig();
        ballSensorCfg.name = "Lift Ball Sensor";
        ballSensorCfg.pin = 40;
        ballSensorCfg.pinMode = PinModeOption::PullUp;
        ballSensorCfg.debounceTimeInMs = 100;
        ballSensorCfg.buttonType = ButtonType::NormalOpen;
        _ballSensor->setConfig(ballSensorCfg);
        addChild(_ballSensor);

        _loader = new Servo(getId() + "-loader");
        auto loaderCfg = _loader->getConfig();
        loaderCfg.name = "Lift Loader";
        loaderCfg.pin = 39;
        loaderCfg.mcpwmChannel = -1;
        loaderCfg.frequency = 50;
        loaderCfg.resolutionBits = 10;
        loaderCfg.minDutyCycle = 9.5f;
        loaderCfg.maxDutyCycle = 5.5f;
        loaderCfg.defaultDurationInMs = 200;
        _loader->setConfig(loaderCfg);
        addChild(_loader);

        _unloader = new Servo(getId() + "-unloader");
        auto unloaderCfg = _unloader->getConfig();
        unloaderCfg.name = "Lift Unloader";
        unloaderCfg.pin = 38;
        unloaderCfg.mcpwmChannel = -1;
        unloaderCfg.frequency = 50;
        unloaderCfg.resolutionBits = 10;
        unloaderCfg.minDutyCycle = 12.2f;
        unloaderCfg.maxDutyCycle = 4.0f;
        unloaderCfg.defaultDurationInMs = 1200;
        _unloader->setConfig(unloaderCfg);
        addChild(_unloader);
    }

    Lift::~Lift()
    {
    }

    void Lift::setup()
    {
        Device::setup();

        _state.state = LiftStateEnum::UNKNOWN;
        _state.errorCode = LiftErrorCode::NONE;
        _state.errorMessage = "";
        _state.onErrorChange = false;

        if (_stepper->getPins().empty())

        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "No pins configured for stepper");
        }

        if (_limitSwitch->getPins().empty())
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "No pins configured for limit switch");
        }

        if (_ballSensor->getPins().empty())
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "No pins configured for ball sensor");
        }

        if (_loader->getPins().empty())
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "No pins configured for loader servo");
        }

        if (_unloader->getPins().empty())
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "No pins configured for unloader servo");
        }

        // Validate configuration
        if (_config.minSteps >= _config.maxSteps)
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "minSteps must be less than maxSteps");
        }
        if (_config.minSteps < 0 || _config.maxSteps < 0)
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "minSteps and maxSteps must be non-negative");
        }
        if (_config.downFactor <= 0.0f)
        {
            setError(LiftErrorCode::LIFT_CONFIGURATION_ERROR, "downFactor must be positive");
        }

        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void Lift::loop()
    {
        Device::loop();

        _state.onErrorChange = false;

        if (_state.state == LiftStateEnum::ERROR)
        {
            _state.errorMessage = ""; // Clear error message when error is resolved
            _state.errorCode = LiftErrorCode::NONE;
        }

        // Check ball sensor state and notify if changed
        // bool ballWaiting = _ballSensor ? _ballSensor->getState().isPressed : false;

        // TODO in semaphore?
        // TODO: stack of errors?

        /*
        bool changed = (_state.isBallWaiting != ballWaiting);
        _state.isBallWaiting = ballWaiting;

        if (changed)
        {
            MLOG_INFO("%s: Ball waiting state changed to %s", toString().c_str(), ballWaiting ? "true" : "false");
            notifyStateChanged();
        }

    }
    */
        // State machine logic
        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        {
            /*
            if (_limitSwitch->hasMixin("state"))
            {
                _stepper->setCurrentPosition(0);

                // Check if at limit - simplified check
                if (_stepper->hasMixin("state"))
                {
                    // Set current position to 0 if at limit
                  //  _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
                  //  notifyStateChanged();
                }
            }
                */
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
            if (millis() - _loadStartTime >= 1000)
            {
                loadBallEnd();
            }
            break;
        case LiftStateEnum::LIFT_DOWN_LOADED:
            break;
        case LiftStateEnum::LIFT_DOWN_UNLOADED:
            break;
        case LiftStateEnum::LIFT_UP_UNLOADING:
            // Wait 2 seconds after starting unload, then end the unloading process
            if (millis() - _unloadStartTime >= 2000)
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
                MLOG_INFO("%s: Top reached", toString().c_str());
                _state.state = _state.isLoaded ? LiftStateEnum::LIFT_UP_LOADED : LiftStateEnum::LIFT_UP_UNLOADED;
                notifyStateChanged();
            }
            break;
        case LiftStateEnum::MOVING_DOWN:
            // When stepper stops moving, return to appropriate idle state
            if (isStepperIdle())
            {
                MLOG_INFO("%s: Movement complete", toString().c_str());
                _state.state = _state.isLoaded ? LiftStateEnum::LIFT_DOWN_LOADED : LiftStateEnum::LIFT_DOWN_UNLOADED;
                notifyStateChanged();
            }
            else if (isAtLimit() && _state.state == LiftStateEnum::MOVING_DOWN)
            {
                MLOG_WARN("%s: Limit switch triggered during downward movement - stopping", toString().c_str());
                stopStepper();
                _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
                notifyStateChanged();
            }
            break;
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in loop(): " + stateToString(_state.state));
        }
    }

    bool Lift::up(float speedRatio)
    {
        bool isSuccess = false;

        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("%s: Cannot move up, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            break;

        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        case LiftStateEnum::LIFT_DOWN_LOADED:
        case LiftStateEnum::MOVING_DOWN:
        case LiftStateEnum::MOVING_UP: // for changed speed
        {
            // Check if lift is already at or above max position
            long currentPos = getCurrentPosition();
            if (currentPos >= _config.maxSteps)
            {
                MLOG_WARN("%s: Cannot move up - already at max position (current: %ld, max: %ld)", toString().c_str(), currentPos, _config.maxSteps);
                break;
            }

            MLOG_INFO("%s: Moving up to %ld steps", toString().c_str(), _config.maxSteps);
            _state.state = LiftStateEnum::MOVING_UP;
            notifyStateChanged();
            isSuccess = moveStepperTo(_config.maxSteps, speedRatio);
            break;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in up(): " + stateToString(_state.state));
            break;
        }

        return isSuccess;
    }

    bool Lift::down(float speedRatio)
    {
        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        case LiftStateEnum::LIFT_DOWN_LOADED:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("%s: Cannot move down, state is %s", toString().c_str(), stateToString(_state.state).c_str());
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
                MLOG_WARN("%s: Cannot move down - already at min position (current: %ld, min: %ld)", toString().c_str(), currentPos, _config.minSteps);
                return false;
            }

            long steps = (_config.minSteps - currentPos) * DOWN_FACTOR;
            _state.state = LiftStateEnum::MOVING_DOWN;
            notifyStateChanged();
            return moveStepper(steps, speedRatio);
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in up()");
            return false;
        }
    }

    bool Lift::init()
    {
        MLOG_INFO("%s: Starting init sequence", toString().c_str());

        _state.state = LiftStateEnum::INIT;
        _state.initStep = 1; // unload end

        notifyStateChanged();
        return true;
    }

    bool Lift::loadBall()
    {
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
            MLOG_WARN("%s: Cannot load ball, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            return false;

        case LiftStateEnum::LIFT_DOWN_UNLOADED:
        {
            bool result = loadBallStart();
            return result;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in down()");
            return false;
        }
    }

    bool Lift::unloadBall()
    {
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
            MLOG_WARN("%s: Cannot unload ball, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            return false;

        case LiftStateEnum::LIFT_UP_UNLOADED:
        case LiftStateEnum::LIFT_UP_LOADED:
        {
            bool result = unloadBallStart();
            return result;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in unloadBall()");
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

    bool Lift::isInitialized() const
    {
        return _state.state != LiftStateEnum::INIT && _state.state != LiftStateEnum::UNKNOWN;
    }

    void Lift::addStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["isBallWaiting"] = _state.isBallWaiting;
        doc["isLoaded"] = _state.isLoaded;
        doc["currentPosition"] = getCurrentPosition();
        doc["errorMessage"] = _state.errorMessage;
        doc["errorCode"] = errorCodeToString(_state.errorCode);
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
            MLOG_WARN("%s: Unknown action '%s'", toString().c_str(), action.c_str());
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

    String Lift::errorCodeToString(LiftErrorCode errorCode) const
    {
        switch (errorCode)
        {
        case LiftErrorCode::NONE:
            return "";
        case LiftErrorCode::LIFT_CONFIGURATION_ERROR:
            return "LIFT_CONFIGURATION_ERROR";
        case LiftErrorCode::LIFT_STATE_ERROR:
            return "LIFT_STATE_ERROR";
        default:
            return "UNKNOWN_ERROR_CODE";
        }
    }

    bool Lift::loadBallStart()
    {
        MLOG_INFO("%s: Loading ball...", toString().c_str());
        _state.state = LiftStateEnum::LIFT_DOWN_LOADING;
        _loadStartTime = millis();
        _state.isLoaded = true;
        notifyStateChanged();

        // Set loader to 100 (fully open) - simplified control
        return _loader->setValue(100);
    }

    bool Lift::loadBallEnd()
    {

        _state.state = LiftStateEnum::LIFT_DOWN_LOADED;
        notifyStateChanged();

        // Set loader to 0 (fully closed) - simplified control
        return _loader->setValue(0);
    }

    bool Lift::unloadBallStart()
    {

        MLOG_INFO("%s: Unloading ball...", toString().c_str());
        _state.state = LiftStateEnum::LIFT_UP_UNLOADING;
        _unloadStartTime = millis();
        _state.isLoaded = false;
        notifyStateChanged();

        // Set unloader to 100 (fully open) - simplified control
        return _unloader->setValue(100);
    }

    bool Lift::unloadBallEnd()
    {

        _state.state = LiftStateEnum::LIFT_UP_UNLOADED;
        notifyStateChanged();

        // Set unloader to 0 (fully closed) - simplified control
        return _unloader->setValue(0);
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

    void Lift::setError(LiftErrorCode errorCode, const String &message)
    {
        MLOG_ERROR("%s: %s - %s", toString().c_str(), errorCodeToString(errorCode).c_str(), message.c_str());
        _state.state = LiftStateEnum::ERROR;
        _state.onErrorChange = true;
        _state.errorMessage = message; // Store the error message
        _state.errorCode = errorCode;  // Store the error code

        notifyStateChanged();
    }

    void Lift::handleInitSequence()
    {
        static long nextInitStepTime = 0;
        if (millis() < nextInitStepTime)
        {
            return; // Wait until next step time
        }

        // Simplified init sequence - would need full implementation
        switch (_state.initStep)
        {
        case 1:
            // Move unload out of the way
            _state.initStep = 2;
            _unloader->setValue(100);
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        case 2:
            _state.initStep = 3;
            unloadBallEnd();
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        case 3:
            // Move slowly down to find limit switch
            _state.initStep = 4;
            long steps = (_config.minSteps - _config.maxSteps) * DOWN_FACTOR;
            moveStepper(steps, 0.5);
            break;
        case 4:
            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }
            // Timeout
            if (millis() + 20000 < nextInitStepTime)
            {
                setError(LiftErrorCode::LIFT_NO_ZERO, "Initialization timeout: limit switch not triggered");
                return; // Wait until next step time
            }
            // load ball
            _state.initStep = 5;
            _loader->setValue(100);
            nextInitStepTime = millis() + _loader->getConfig().defaultDurationInMs;
            break;
        /*case 5:
            // TODO
            _state.initStep = 6;
            loadBallEnd();
            break;
        case 6:
            // Move ball up
            _state.initStep = 7;
            up();
        case 7:
            _state.initStep = 8;
            unloadBallStart();
            break;
        case 8:
            _state.initStep = 9;
            unloadBallEnd();
            break;
        case 9:
            _state.initStep = 10;
            down();
            break;
            */
        default:
            _state.state = LiftStateEnum::LIFT_DOWN_UNLOADED;
            _state.initStep = 0;
            notifyStateChanged();
            break;
        }
    }

} // namespace devices
