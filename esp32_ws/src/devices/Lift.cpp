#include "devices/Lift.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "devices/Servo.h"
#include "Logging.h"

namespace devices
{

    /* Move 2% extra down */
    const float DOWN_FACTOR = 1.005f;                // Move 0.5% extra when going down to ensure full descent
    const float IMMEDIATE_DECELERATION = 1000000.0f; // Very high deceleration for immediate stop

    Lift::Lift(const String &id)
        : Device(id, "lift")
    {
        // See Device.h "Two-phase initialization contract".
        applyDefaultConfig();
    }

    Lift::~Lift()
    {
    }

    void Lift::applyDefaultConfig()
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
        // Pins will be configured by parent MarbleController
        _stepper->setConfig(stepperCfg);
        addChild(_stepper);

        _limitSwitch = new Button(getId() + "-limit");
        auto limitSwitchCfg = _limitSwitch->getConfig();
        limitSwitchCfg.name = "Lift Limit Switch";
        // Pin will be configured by parent MarbleController
        _limitSwitch->setConfig(limitSwitchCfg);
        addChild(_limitSwitch);

        _ballSensor = new Button(getId() + "-ball-sensor");
        auto ballSensorCfg = _ballSensor->getConfig();
        ballSensorCfg.name = "Lift Ball Sensor";
        // Pin will be configured by parent MarbleController
        _ballSensor->setConfig(ballSensorCfg);
        addChild(_ballSensor);

        _loader = new Servo(getId() + "-loader");
        auto loaderCfg = _loader->getConfig();
        loaderCfg.name = "Lift Loader";
        // Pin will be configured by parent MarbleController
        _loader->setConfig(loaderCfg);
        addChild(_loader);

        _unloader = new Servo(getId() + "-unloader");
        auto unloaderCfg = _unloader->getConfig();
        unloaderCfg.name = "Lift Unloader";
        // Pin will be configured by parent MarbleController
        _unloader->setConfig(unloaderCfg);
        addChild(_unloader);
    }

    void Lift::setup()
    {
        Device::setup();

        _state.state = LiftStateEnum::UNKNOWN;
        _state.onErrorChange = false;
        _loadEndTime = 0;

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

    void Lift::teardown()
    {
        Device::teardown();

        _state.state = LiftStateEnum::UNKNOWN;
        _state.onErrorChange = false;
        _state.ballWaitingSince = 0;
        _state.isLoaded = false;
        _state.initStep = 0;
        _initSpeedRatio = 1.0f;

        _loadStartTime = 0;
        _loadEndTime = 0;
        _unloadStartTime = 0;
        _unloadEndTime = 0;
        _unloadDurationMs = 0;
        _stepperStartTime = 0;
    }

    void Lift::loop()
    {
        Device::loop();

        _state.onErrorChange = false;

        // Check if any required child servo is in error; skip state machine if so
        if (checkChildErrors())
            return;

        // Check ball sensor state and notify if changed
        bool ballWaiting = _ballSensor ? _ballSensor->getState().isPressed : false;

        // TODO in semaphore?
        // TODO: stack of errors?

        bool wasWaiting = (_state.ballWaitingSince > 0);
        bool changed = (wasWaiting != ballWaiting);

        if (ballWaiting && !wasWaiting)
        {
            // Ball started waiting - record timestamp
            _state.ballWaitingSince = millis();
        }
        else if (!ballWaiting && wasWaiting)
        {
            // Ball stopped waiting - reset timestamp
            _state.ballWaitingSince = 0;
        }

        if (changed)
        {
            MLOG_INFO("%s: Ball waiting state changed to %s", toString().c_str(), ballWaiting ? "true" : "false");
            notifyStateChanged();
        }

        // State machine logic
        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        {
            break;
        }
        case LiftStateEnum::INIT:
            // Handle reset sequence steps
            initLoop();
            break;
        case LiftStateEnum::ERROR:
            // In error state, do nothing - requires manual reset
            break;
        case LiftStateEnum::LIFT_DOWN_LOADING:
            // Wait 1 second after starting load, then end the loading process
            if (millis() - _loadStartTime >= _loader->getConfig().defaultDurationInMs + 500)
            {
                loadBallEnd();
            }
            break;
        case LiftStateEnum::LIFT_DOWN:
            break;
        case LiftStateEnum::LIFT_UP_UNLOADING:
            // Wait 2 seconds after starting unload, then end the unloading process
            if (millis() - _unloadStartTime >= _unloadDurationMs + 200)
            {
                unloadBallEnd(1.0f);
            }
            break;
        case LiftStateEnum::LIFT_UP:
            break;
        case LiftStateEnum::MOVING_UP:
            if (!_stepper->getState().isMoving && (millis() > _stepperStartTime + 10))
            {
                MLOG_INFO("%s: Top reached", toString().c_str());
                _state.state = LiftStateEnum::LIFT_UP;
                _stepperStartTime = 0;
                notifyStateChanged();
            }
            break;
        case LiftStateEnum::MOVING_DOWN:
            if (!_stepper->getState().isMoving && (millis() > _stepperStartTime + 10))
            {
                setError(LiftErrorCode::LIFT_NO_ZERO, "limit switch not triggered when moving down");
                return;
            }

            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }

            MLOG_DEBUG("%s: Reached bottom with limit switch", toString().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop(IMMEDIATE_DECELERATION);
            _stepperStartTime = 0;
            _state.state = LiftStateEnum::LIFT_DOWN;
            notifyStateChanged();
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
        case LiftStateEnum::LIFT_UP:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("%s: Cannot move up, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            break;

        case LiftStateEnum::LIFT_DOWN:
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
            isSuccess = moveStepperTo(_config.maxSteps, speedRatio);
            notifyStateChanged();
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
        bool isSuccess = false;
        switch (_state.state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::INIT:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::LIFT_DOWN:
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_UP_UNLOADING:
            MLOG_WARN("%s: Cannot move down, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            isSuccess = false;
            break;

        case LiftStateEnum::LIFT_UP:
        case LiftStateEnum::MOVING_DOWN: // for changed speed
        case LiftStateEnum::MOVING_UP:
        {
            const bool wasMovingDown = (_state.state == LiftStateEnum::MOVING_DOWN);

            // Check if lift is already at or below min position
            long currentPos = getCurrentPosition();
            if (currentPos <= _config.minSteps)
            {
                MLOG_WARN("%s: Cannot move down - already at min position (current: %ld, min: %ld)", toString().c_str(), currentPos, _config.minSteps);
                isSuccess = false;
                break;
            }

            _state.state = LiftStateEnum::MOVING_DOWN;

            if (wasMovingDown)
            {
                long targetPos = _stepper->getState().targetPosition;
                if (targetPos >= currentPos)
                {
                    long steps = (_config.minSteps - currentPos) * DOWN_FACTOR;
                    targetPos = currentPos + steps;
                }
                isSuccess = moveStepperTo(targetPos, speedRatio);
            }
            else
            {
                long steps = (_config.minSteps - currentPos) * DOWN_FACTOR;
                isSuccess = moveStepper(steps, speedRatio);
            }

            notifyStateChanged();
            break;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in up()");
            isSuccess = false;
            break;
        }
        return isSuccess;
    }

    bool Lift::init(float speedRatio)
    {
        MLOG_INFO("%s: Starting init sequence with speed ratio %.2f", toString().c_str(), speedRatio);

        _state.state = LiftStateEnum::INIT;
        _state.initStep = 1; // unload end
        _initSpeedRatio = speedRatio;

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
        case LiftStateEnum::LIFT_UP:
        case LiftStateEnum::LIFT_UP_UNLOADING:
        case LiftStateEnum::MOVING_DOWN:
            MLOG_WARN("%s: Cannot load ball, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            return false;

        case LiftStateEnum::LIFT_DOWN:
        {
            if (_state.isLoaded)
            {
                MLOG_WARN("%s: Cannot load ball, already loaded", toString().c_str());
                return false;
            }
            bool result = loadBallStart();
            return result;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in down()");
            return false;
        }
    }

    bool Lift::unloadBall(float durationRatio)
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
        case LiftStateEnum::LIFT_DOWN:
            MLOG_WARN("%s: Cannot unload ball, state is %s", toString().c_str(), stateToString(_state.state).c_str());
            return false;

        case LiftStateEnum::LIFT_UP:
        {
            bool result = unloadBallStart(durationRatio);
            return result;
        }
        default:
            setError(LiftErrorCode::LIFT_STATE_ERROR, "Unknown state encountered in unloadBall()");
            return false;
        }
    }

    bool Lift::isBallWaiting() const
    {
        return _state.ballWaitingSince > 0;
    }

    bool Lift::isLoaded() const
    {
        return _state.isLoaded;
    }

    bool Lift::isInitialized() const
    {
        return _state.state != LiftStateEnum::INIT && _state.state != LiftStateEnum::UNKNOWN;
    }

    void Lift::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["ballWaitingSince"] = _state.ballWaitingSince;
        doc["isLoaded"] = _state.isLoaded;
        // Only include currentPosition when stepper is not moving to avoid constant notifications
        if (_state.state != LiftStateEnum::MOVING_UP && _state.state != LiftStateEnum::MOVING_DOWN)
        {
            doc["currentPosition"] = getCurrentPosition();
        }
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
            float durationRatio = 1.0f;
            if (args && (*args)["durationRatio"].is<float>())
            {
                durationRatio = (*args)["durationRatio"];
            }
            return unloadBall(durationRatio);
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
        case LiftStateEnum::LIFT_DOWN:
            return "LiftDown";
        case LiftStateEnum::LIFT_UP_UNLOADING:
            return "LiftUpUnloading";
        case LiftStateEnum::LIFT_UP:
            return "LiftUp";
        case LiftStateEnum::MOVING_UP:
            return "MovingUp";
        case LiftStateEnum::MOVING_DOWN:
            return "MovingDown";
        default:
            MLOG_ERROR("%s: Unknown lift state code in stateToString: %d", toString().c_str(), static_cast<int>(state));
            return "Unknown";
        }
    }

    String Lift::errorCodeToString(LiftErrorCode errorCode) const
    {
        switch (errorCode)
        {
        case LiftErrorCode::NONE:
            return "";
        case LiftErrorCode::LIFT_INIT_NO_ZERO:
            return "LIFT_INIT_NO_ZERO";
        case LiftErrorCode::LIFT_CONFIGURATION_ERROR:
            return "LIFT_CONFIGURATION_ERROR";
        case LiftErrorCode::LIFT_STATE_ERROR:
            return "LIFT_STATE_ERROR";
        case LiftErrorCode::LIFT_NO_ZERO:
            return "LIFT_NO_ZERO";
        case LiftErrorCode::LIFT_CHILD_ERROR:
            return "LIFT_CHILD_ERROR";
        default:
            MLOG_ERROR("%s: Unknown Lift error code in errorCodeToString: %d", toString().c_str(), static_cast<int>(errorCode));
            return "UNKNOWN_ERROR_CODE";
        }
    }

    bool Lift::loadBallStart()
    {
        MLOG_INFO("%s: Loading ball...", toString().c_str());
        _state.state = LiftStateEnum::LIFT_DOWN_LOADING;
        _loadStartTime = millis();
        _loadEndTime = 0;
        _state.isLoaded = true;
        notifyStateChanged();

        // Set loader to 100 (fully open) - simplified control
        return _loader->setValue(100);
    }

    bool Lift::loadBallEnd()
    {
        // Phase 1: start closing the loader once the loading phase is complete
        if (_loadEndTime == 0)
        {
            _loadEndTime = millis();
            return _loader->setValue(0, static_cast<int>(_loader->getConfig().defaultDurationInMs));
        }

        // Phase 2: wait until the close animation has completed before reporting LIFT_DOWN
        if (millis() - _loadEndTime < _loader->getConfig().defaultDurationInMs)
        {
            return true;
        }

        // Disable the load servo after the closing motion finishes
        _loader->disable();

        _state.state = LiftStateEnum::LIFT_DOWN;
        _loadStartTime = 0;
        _loadEndTime = 0;
        notifyStateChanged();
        return true;
    }

    bool Lift::unloadBallStart(float durationRatio)
    {

        if (durationRatio <= 0.0f)
        {
            durationRatio = 1.0f;
        }

        const uint32_t baseDuration = _unloader->getConfig().defaultDurationInMs;
        uint32_t scaledDuration = static_cast<uint32_t>(static_cast<float>(baseDuration) * durationRatio);
        if (scaledDuration == 0)
        {
            scaledDuration = 1;
        }

        MLOG_INFO("%s: Unloading ball...", toString().c_str());
        _state.state = LiftStateEnum::LIFT_UP_UNLOADING;
        _unloadStartTime = millis();
        _unloadEndTime = 0;
        _unloadDurationMs = scaledDuration;
        notifyStateChanged();

        // Set unloader to 100 (fully open) - with duration
        return _unloader->setValue(100, static_cast<int>(_unloadDurationMs));
    }

    bool Lift::unloadBallEnd(float durationRatio)
    {
        // Phase 1: start closing the unloader once opening phase is done
        if (_unloadEndTime == 0)
        {
            _unloadEndTime = millis();
            return _unloader->setValue(0, static_cast<int>(_unloadDurationMs));
        }

        // Phase 2: wait until closing animation has completed before reporting LIFT_UP
        if (millis() - _unloadEndTime < _unloadDurationMs)
        {
            return true;
        }

        // Turn off PWM to save power and reduce heat - simplified control
        _unloader->disable();

        _state.state = LiftStateEnum::LIFT_UP;
        _state.isLoaded = false;
        _unloadStartTime = 0;
        _unloadEndTime = 0;
        _unloadDurationMs = 0;
        notifyStateChanged();
        return true;
    }

    // Helper methods for stepper control - simplified implementations
    long Lift::getCurrentPosition() const
    {
        return _stepper->getState().currentPosition;
    }

    bool Lift::moveStepper(long steps, float speedRatio)
    {
        _stepper->move(steps, _stepper->getConfig().defaultSpeed * speedRatio);
        _stepperStartTime = millis();
        return true;
    }

    bool Lift::moveStepperTo(long position, float speedRatio)
    {
        _stepper->moveTo(position, _stepper->getConfig().defaultSpeed * speedRatio);
        _stepperStartTime = millis();
        return true;
    }

    bool Lift::stopStepper()
    {
        _stepper->stop();
        _stepperStartTime = 0;
        return true;
    }

    void Lift::setError(LiftErrorCode errorCode, const String &message)
    {
        _state.state = LiftStateEnum::ERROR;
        _state.onErrorChange = true;
        Device::setError(errorCodeToString(errorCode), message);

        notifyStateChanged();
    }

    bool Lift::checkChildErrors()
    {
        if (_state.state == LiftStateEnum::ERROR)
        {
            // Auto-clear if we were in child error and children have since recovered
            if (getErrorCode() == errorCodeToString(LiftErrorCode::LIFT_CHILD_ERROR))
            {
                const bool loaderInError = _loader != nullptr && _loader->getState().state == ServoStateEnum::ERROR;
                const bool unloaderInError = _unloader != nullptr && _unloader->getState().state == ServoStateEnum::ERROR;
                if (!loaderInError && !unloaderInError)
                {
                    MLOG_INFO("%s: Child servo error resolved, clearing error", toString().c_str());
                    _state.state = LiftStateEnum::UNKNOWN;
                    _state.onErrorChange = true;
                    Device::clearError();
                    notifyStateChanged();
                    return false;
                }
                return true; // Still in child error
            }
            return false; // Error from a different cause, not a child error
        }

        // Check loader servo
        if (_loader != nullptr && _loader->getState().state == ServoStateEnum::ERROR)
        {
            String message = "required child 'loader' has an error: " + _loader->getErrorMessage();
            setError(LiftErrorCode::LIFT_CHILD_ERROR, message);
            return true;
        }

        // Check unloader servo
        if (_unloader != nullptr && _unloader->getState().state == ServoStateEnum::ERROR)
        {
            String message = "required child 'unloader' has an error: " + _unloader->getErrorMessage();
            setError(LiftErrorCode::LIFT_CHILD_ERROR, message);
            return true;
        }

        return false;
    }

    void Lift::initLoop()
    {
        // wait between steps
        static long nextInitStepTime = 0;
        if (millis() < nextInitStepTime)
        {
            return; // Wait until next step time
        }

        // Simplified init sequence - would need full implementation
        switch (_state.initStep)
        {
        case 1:
        {
            // Move unload out of the way
            MLOG_DEBUG("%s: Init step 1: Unloading start", toString().c_str());
            _state.initStep = 2;
            auto duration = _unloader->getConfig().defaultDurationInMs;
            _unloader->setValue(100, duration);
            nextInitStepTime = millis() + duration + 50;
            break;
        }
        case 2:
        {
            MLOG_DEBUG("%s: Init step 2: Unloading end", toString().c_str());
            _state.initStep = 3;
            auto duration = _unloader->getConfig().defaultDurationInMs;
            _unloader->setValue(0, duration);
            nextInitStepTime = millis() + duration + 300;
            break;
        }
        case 3:
        {
            MLOG_DEBUG("%s: Init step 3: Moving down to find limit switch", toString().c_str());
            // Move slowly down to find limit switch
            _state.initStep = 4;
            long steps = (_config.minSteps - _config.maxSteps) * DOWN_FACTOR;
            moveStepper(steps, 0.3f * _initSpeedRatio);
            nextInitStepTime = millis() + 100;
            break;
        }
        case 4:
        {
            if (!_stepper->getState().isMoving && _limitSwitch && !_limitSwitch->getState().isPressed)
            {
                setError(LiftErrorCode::LIFT_INIT_NO_ZERO, "Initialization failed: limit switch not triggered");
                return;
            }

            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }

            MLOG_DEBUG("%s: Init step 4: Bottom reached, stopping stepper", toString().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop(IMMEDIATE_DECELERATION);
            _stepperStartTime = 0;

            // go to wait step
            _state.initStep = 5;
            nextInitStepTime = millis() + 10;
            break;
        }
        case 5:
        {
            // Wait for stepper to stop
            if (_stepper->getState().isMoving)
            {
                return;
            }

            MLOG_DEBUG("%s: Init step 5: Moving back up after bottom reached to unload possible ball in lift", toString().c_str());
            _state.initStep = 6;
            moveStepperTo(_config.maxSteps, _initSpeedRatio);
            nextInitStepTime = millis() + 10; // wait until move started
            break;
        }
        case 6:
        {
            // Wait until top reached
            if (_stepper->getState().isMoving)
            {
                return; // Wait until move completed
            }

            MLOG_DEBUG("%s: Init step 6: Unloading start", toString().c_str());
            _state.initStep = 7;
            auto duration = _unloader->getConfig().defaultDurationInMs / 2;
            _unloader->setValue(100, duration);
            nextInitStepTime = millis() + duration + 300;
            break;
        }
        case 7:
        {
            MLOG_DEBUG("%s: Init step 7: Unloading end", toString().c_str());
            _state.initStep = 8;
            auto duration = _unloader->getConfig().defaultDurationInMs;
            _unloader->setValue(0, duration);
            nextInitStepTime = millis() + duration + 300;
            break;
        }
        case 8:
        {
            MLOG_DEBUG("%s: Init step 8: Moving back down until limit switch", toString().c_str());
            _state.initStep = 9;
            long steps = (_config.minSteps - _config.maxSteps) * DOWN_FACTOR;
            moveStepper(steps, _initSpeedRatio);
            nextInitStepTime = millis() + 100;
            break;
        }
        case 9:
        {
            if (!_stepper->getState().isMoving && _limitSwitch && !_limitSwitch->getState().isPressed)
            {
                setError(LiftErrorCode::LIFT_INIT_NO_ZERO, "Initialization failed: limit switch not triggered");
                return;
            }

            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }

            MLOG_DEBUG("%s: Init step 9: Loading start", toString().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop(IMMEDIATE_DECELERATION);
            _stepperStartTime = 0;

            // Init complete
            MLOG_INFO("%s: Initialization complete", toString().c_str());
            _state.state = LiftStateEnum::LIFT_DOWN;
            _state.initStep = 0;
            notifyStateChanged();
            break;
        }
        }
    }

} // namespace devices
