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
        bool ballWaiting = _ballSensor ? _ballSensor->getState().isPressed : false;

        // TODO in semaphore?
        // TODO: stack of errors?

        bool changed = (_state.isBallWaiting != ballWaiting);
        _state.isBallWaiting = ballWaiting;

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
            if (millis() - _unloadStartTime >= _unloader->getConfig().defaultDurationInMs + 1000)
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
            _stepper->stop(100000);
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
            // Check if lift is already at or below min position
            long currentPos = getCurrentPosition();
            if (currentPos <= _config.minSteps)
            {
                MLOG_WARN("%s: Cannot move down - already at min position (current: %ld, min: %ld)", toString().c_str(), currentPos, _config.minSteps);
                isSuccess = false;
                break;
            }

            long steps = (_config.minSteps - currentPos) * DOWN_FACTOR;
            _state.state = LiftStateEnum::MOVING_DOWN;
            isSuccess = moveStepper(steps, speedRatio);
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
        // Only include currentPosition when stepper is not moving to avoid constant notifications
        if (_state.state != LiftStateEnum::MOVING_UP && _state.state != LiftStateEnum::MOVING_DOWN)
        {
            doc["currentPosition"] = getCurrentPosition();
        }
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
        case LiftErrorCode::LIFT_CONFIGURATION_ERROR:
            return "LIFT_CONFIGURATION_ERROR";
        case LiftErrorCode::LIFT_STATE_ERROR:
            return "LIFT_STATE_ERROR";
        case LiftErrorCode::LIFT_NO_ZERO:
            return "LIFT_NO_ZERO";
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
        _state.isLoaded = true;
        notifyStateChanged();

        // Set loader to 100 (fully open) - simplified control
        return _loader->setValue(100);
    }

    bool Lift::loadBallEnd()
    {

        _state.state = LiftStateEnum::LIFT_DOWN;
        notifyStateChanged();

        // Set loader to 0 (fully closed) - simplified control
        return _loader->setValue(0);
    }

    bool Lift::unloadBallStart(float durationRatio)
    {

        MLOG_INFO("%s: Unloading ball...", toString().c_str());
        _state.state = LiftStateEnum::LIFT_UP_UNLOADING;
        _unloadStartTime = millis();
        notifyStateChanged();

        // Set unloader to 100 (fully open) - with duration
        int durationMs = _unloader->getConfig().defaultDurationInMs * durationRatio;
        return _unloader->setValue(100, durationMs);
    }

    bool Lift::unloadBallEnd(float durationRatio)
    {

        _state.state = LiftStateEnum::LIFT_UP;
        _state.isLoaded = false;
        notifyStateChanged();

        // Set unloader to 0 (fully closed) - with duration
        int durationMs = _unloader->getConfig().defaultDurationInMs * durationRatio;
        return _unloader->setValue(0, durationMs);
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
        MLOG_ERROR("%s: %s - %s", toString().c_str(), errorCodeToString(errorCode).c_str(), message.c_str());
        _state.state = LiftStateEnum::ERROR;
        _state.onErrorChange = true;
        _state.errorMessage = message; // Store the error message
        _state.errorCode = errorCode;  // Store the error code

        notifyStateChanged();
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
            _unloader->setValue(100);
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        }
        case 2:
        {
            MLOG_DEBUG("%s: Init step 2: Unloading end", toString().c_str());
            _state.initStep = 3;
            _unloader->setValue(0);
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        }
        case 3:
        {
            MLOG_DEBUG("%s: Init step 3: Moving down to find limit switch", toString().c_str());
            // Move slowly down to find limit switch
            _state.initStep = 4;
            long steps = (_config.minSteps - _config.maxSteps) * DOWN_FACTOR;
            moveStepper(steps, 0.5);
            nextInitStepTime = millis() + 100;
            break;
        }
        case 4:
        {
            if (!_stepper->getState().isMoving)
            {
                setError(LiftErrorCode::LIFT_NO_ZERO, "Initialization failed: limit switch not triggered");
                return;
            }

            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }

            MLOG_DEBUG("%s: Init step 4: Loading start", toString().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop(100000);
            _stepperStartTime = 0;

            // load ball
            _state.initStep = 5;
            _loader->setValue(100);
            nextInitStepTime = millis() + _loader->getConfig().defaultDurationInMs;
            break;
        }
        case 5:
        {
            MLOG_DEBUG("%s: Init step 5: Loading end", toString().c_str());
            _state.initStep = 6;
            _loader->setValue(0);
            nextInitStepTime = millis() + _loader->getConfig().defaultDurationInMs + 500;
            break;
        }
        case 6:
        {
            MLOG_DEBUG("%s: Init step 6: Moving possible loaded lift up", toString().c_str());
            _state.initStep = 7;
            moveStepperTo(_config.maxSteps, _stepper->getConfig().defaultSpeed * 0.5f);
            nextInitStepTime = millis() + 10; // wait until move started
            break;
        }
        case 7:
        {
            // Wait until top reached
            if (_stepper->getState().isMoving)
            {
                return; // Wait until move completed
            }

            MLOG_DEBUG("%s: Init step 7: Unloading start", toString().c_str());
            _state.initStep = 8;
            _unloader->setValue(100);
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        }
        case 8:
        {
            MLOG_DEBUG("%s: Init step 8: Unloading end", toString().c_str());
            _state.initStep = 9;
            _unloader->setValue(0);
            nextInitStepTime = millis() + _unloader->getConfig().defaultDurationInMs;
            break;
        }
        case 9:
        {
            MLOG_DEBUG("%s: Init step 9: Moving back down until limit switch", toString().c_str());
            _state.initStep = 10;
            long steps = (_config.minSteps - _config.maxSteps) * DOWN_FACTOR;
            moveStepper(steps, _stepper->getConfig().defaultSpeed);
            nextInitStepTime = millis() + 100;
            break;
        }
        case 10:
        {
            if (!_stepper->getState().isMoving)
            {
                setError(LiftErrorCode::LIFT_NO_ZERO, "Initialization failed: limit switch not triggered");
                return; // Wait until next step time
            }

            // wait until down
            if (!_limitSwitch->getState().isPressed)
            {
                return;
            }

            _stepper->setCurrentPosition(0);
            _stepper->stop(100000);

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
