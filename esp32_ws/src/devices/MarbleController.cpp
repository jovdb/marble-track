#include "devices/MarbleController.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"
#include "devices/Stepper.h"

extern DeviceManager deviceManager;

namespace devices
{

    MarbleController::MarbleController(const String &id) : Device(id, "marblecontroller")
    {
        _buzzer = new devices::Buzzer("buzzer");
        auto buzzerConfig = _buzzer->getConfig();
        buzzerConfig.name = "Buzzer";
        buzzerConfig.pin = 14;
        _buzzer->setConfig(buzzerConfig);
        addChild(_buzzer);

        _lift = new devices::Lift("lift");
        auto liftConfig = _lift->getConfig();
        liftConfig.name = "Lift";
        liftConfig.minSteps = 0;
        liftConfig.maxSteps = 1500;
        liftConfig.downFactor = 1.015;
        _lift->setConfig(liftConfig);
        addChild(_lift);

        // Configure lift's child device pins
        auto liftStepper = _lift->getChildByIdAs<devices::Stepper>("lift-stepper");
        if (liftStepper)
        {
            auto liftStepperConfig = liftStepper->getConfig();
            liftStepperConfig.name = "Lift Stepper";
            liftStepperConfig.stepperType = "DRIVER";
            liftStepperConfig.maxSpeed = 400;
            liftStepperConfig.maxAcceleration = 100;
            liftStepperConfig.defaultSpeed = 150;
            liftStepperConfig.defaultAcceleration = 50;
            liftStepperConfig.stepPin.pin = 1;
            liftStepperConfig.stepPin.expanderId = "";
            liftStepperConfig.dirPin.pin = 2;
            liftStepperConfig.dirPin.expanderId = "";
            liftStepperConfig.enablePin.pin = 42;
            liftStepperConfig.enablePin.expanderId = "";
            liftStepperConfig.invertEnable = true;
            liftStepper->setConfig(liftStepperConfig);
        }

        auto liftLimitSwitch = _lift->getChildByIdAs<devices::Button>("lift-limit");
        if (liftLimitSwitch)
        {
            auto liftLimitSwitchConfig = liftLimitSwitch->getConfig();
            liftLimitSwitchConfig.pinConfig.pin = -1;
            liftLimitSwitchConfig.pinConfig.expanderId = "";
            liftLimitSwitchConfig.pinMode = PinModeOption::PullUp;
            liftLimitSwitch->setConfig(liftLimitSwitchConfig);
        }

        auto liftBallSensor = _lift->getChildByIdAs<devices::Button>("lift-ball-sensor");
        if (liftBallSensor)
        {
            auto liftBallSensorConfig = liftBallSensor->getConfig();
            liftBallSensorConfig.pinConfig.pin = -1;
            liftBallSensorConfig.pinConfig.expanderId = "";
            liftBallSensorConfig.pinMode = PinModeOption::PullUp;
            liftBallSensor->setConfig(liftBallSensorConfig);
        }

        auto liftLoader = _lift->getChildByIdAs<devices::Servo>("lift-loader");
        if (liftLoader)
        {
            auto liftLoaderConfig = liftLoader->getConfig();
            liftLoaderConfig.pin = 39;
            liftLoaderConfig.mcpwmChannel = 0;
            liftLoaderConfig.frequency = 50;
            liftLoaderConfig.resolutionBits = 10;
            liftLoaderConfig.minDutyCycle = 9.5f;
            liftLoaderConfig.maxDutyCycle = 5.5f;
            liftLoaderConfig.defaultDurationInMs = 200;
            liftLoader->setConfig(liftLoaderConfig);
        }

        auto liftUnloader = _lift->getChildByIdAs<devices::Servo>("lift-unloader");
        if (liftUnloader)
        {
            auto liftUnloaderConfig = liftUnloader->getConfig();
            liftUnloaderConfig.pin = 38;
            liftUnloaderConfig.mcpwmChannel = 1;
            liftUnloaderConfig.frequency = 50;
            liftUnloaderConfig.resolutionBits = 10;
            liftUnloaderConfig.minDutyCycle = 12.2f;
            liftUnloaderConfig.maxDutyCycle = 4.0f;
            liftUnloaderConfig.defaultDurationInMs = 1200;
            liftUnloader->setConfig(liftUnloaderConfig);
        }

        _liftLed = new devices::Led("lift-led");
        auto liftLedConfig = _liftLed->getConfig();
        liftLedConfig.name = "Lift Led";
        liftLedConfig.pinConfig.pin = 0;
        liftLedConfig.pinConfig.expanderId = "ex";
        liftLedConfig.initialState = "OFF";
        _liftLed->setConfig(liftLedConfig);
        addChild(_liftLed);

        _liftBtn = new devices::Button("lift-btn");
        auto liftButtonConfig = _liftBtn->getConfig();
        liftButtonConfig.name = "Lift Button";
        liftButtonConfig.pinConfig.pin = -1;
        liftButtonConfig.pinConfig.expanderId = "";
        liftButtonConfig.pinMode = PinModeOption::PullUp;
        liftButtonConfig.debounceTimeInMs = 50;
        liftButtonConfig.buttonType = ButtonType::NormalOpen;
        _liftBtn->setConfig(liftButtonConfig);
        addChild(_liftBtn);

        _manualButton = new devices::Button("manual-btn");
        auto manualButtonConfig = _manualButton->getConfig();
        manualButtonConfig.name = "Manual Mode Button";
        manualButtonConfig.pinConfig.pin = -1;
        manualButtonConfig.pinConfig.expanderId = "";
        manualButtonConfig.pinMode = PinModeOption::PullUp;
        manualButtonConfig.debounceTimeInMs = 50;
        manualButtonConfig.buttonType = ButtonType::NormalOpen;
        _manualButton->setConfig(manualButtonConfig);
        addChild(_manualButton);

        // Create wheel with proper config
        _wheel = new devices::Wheel("wheel");

        auto wheelConfig = _wheel->getConfig();
        wheelConfig.name = "Wheel";
        wheelConfig.stepsPerRevolution = 138259;
        wheelConfig.maxStepsPerRevolution = 150000;
        wheelConfig.zeroPointDegree = 180;
        wheelConfig.direction = 1;
        wheelConfig.breakPoints = {82.0f, 160.0f, 285.0f, 317.0f};
        _wheel->setConfig(wheelConfig);
        addChild(_wheel);

        // Get wheel's child devices and configure them
        auto wheelStepper = _wheel->getChildByIdAs<devices::Stepper>("wheel-stepper");
        if (wheelStepper)
        {
            auto wheelStepperConfig = wheelStepper->getConfig();
            wheelStepperConfig.name = "wheel Stepper";
            wheelStepperConfig.stepperType = "DRIVER";
            wheelStepperConfig.maxSpeed = 3000;
            wheelStepperConfig.maxAcceleration = 3000;
            wheelStepperConfig.defaultSpeed = 1000;
            wheelStepperConfig.defaultAcceleration = 200;
            wheelStepperConfig.stepPin.pin = 4;
            wheelStepperConfig.stepPin.expanderId = "";
            wheelStepperConfig.dirPin.pin = 5;
            wheelStepperConfig.dirPin.expanderId = "";
            wheelStepperConfig.enablePin.pin = 6;
            wheelStepperConfig.enablePin.expanderId = "";
            wheelStepperConfig.invertEnable = true;
            wheelStepper->setConfig(wheelStepperConfig);
        }

        auto wheelSensor = _wheel->getChildByIdAs<devices::Button>("wheel-zero-sensor");
        if (wheelSensor)
        {
            auto wheelSensorConfig = wheelSensor->getConfig();
            wheelSensorConfig.name = "Wheel Zero Sensor";
            wheelSensorConfig.pinConfig.pin = -1;
            wheelSensorConfig.pinConfig.expanderId = "";
            wheelSensorConfig.pinMode = PinModeOption::PullUp;
            wheelSensorConfig.debounceTimeInMs = 50;
            wheelSensorConfig.buttonType = ButtonType::NormalOpen;
            wheelSensor->setConfig(wheelSensorConfig);
        }

        // Add wheel button LED
        _wheelLed = new devices::Led("wheel-led");
        auto wheelBtnLedConfig = _wheelLed->getConfig();
        wheelBtnLedConfig.name = "";
        wheelBtnLedConfig.pinConfig.pin = 1;
        wheelBtnLedConfig.pinConfig.expanderId = "ex";
        wheelBtnLedConfig.initialState = "OFF";
        _wheelLed->setConfig(wheelBtnLedConfig);
        addChild(_wheelLed);

        // Add wheel next button
        _wheelBtn = new devices::Button("wheel-btn");
        auto wheelNextBtnConfig = _wheelBtn->getConfig();
        wheelNextBtnConfig.name = "";
        wheelNextBtnConfig.pinConfig.pin = -1;
        wheelNextBtnConfig.pinConfig.expanderId = "";
        wheelNextBtnConfig.pinMode = PinModeOption::PullUp;
        wheelNextBtnConfig.debounceTimeInMs = 50;
        wheelNextBtnConfig.buttonType = ButtonType::NormalOpen;
        _wheelBtn->setConfig(wheelNextBtnConfig);
        addChild(_wheelBtn);

        _spiralLed = new devices::Led("spiral-led");
        auto spiralLedConfig = _spiralLed->getConfig();
        spiralLedConfig.name = "Spiral Led";
        spiralLedConfig.pinConfig.pin = -1;
        spiralLedConfig.pinConfig.expanderId = "";
        spiralLedConfig.initialState = "OFF";
        _spiralLed->setConfig(spiralLedConfig);
        addChild(_spiralLed);

        _spiralBtn = new devices::Button("spiral-btn");
        auto spiralButtonConfig = _spiralBtn->getConfig();
        spiralButtonConfig.name = "Spiral Button";
        spiralButtonConfig.pinConfig.pin = -1;
        spiralButtonConfig.pinConfig.expanderId = "";
        spiralButtonConfig.pinMode = PinModeOption::PullUp;
        spiralButtonConfig.debounceTimeInMs = 50;
        spiralButtonConfig.buttonType = ButtonType::NormalOpen;
        _spiralBtn->setConfig(spiralButtonConfig);
        addChild(_spiralBtn);
    }

    void MarbleController::setup()
    {
        Device::setup();

        // Set auto mode based on manual button state during setup
        isAutoMode = !_manualButton->getState().isPressed;

        // Log the operating mode
        MLOG_INFO("%s Initialized in %s mode", toString().c_str(), isAutoMode ? "AUTO" : "MANUAL");

        playStartupSound();
    }

    void MarbleController::loop()
    {
        Device::loop();

        if (isAutoMode)
        {
            loopAutoLift();
            loopAutoWheel();
            loopAutoSpiral();
        }
        else
        {
            loopManualLift();
            loopManualWheel();
            loopManualSpiral();
        }
    }

    void MarbleController::loopManualLift()
    {

        auto liftState = _lift->getState();

        // LED
        switch (liftState.state)
        {
        case devices::LiftStateEnum::ERROR:
        {
            blinkError(_liftLed);
            break;
        }
        case devices::LiftStateEnum::INIT:
        {
            _liftLed->blink(240, 240);
            break;
        }
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN:
            _liftLed->blink(480, 480);
            break;
        case devices::LiftStateEnum::UNKNOWN:
        case devices::LiftStateEnum::LIFT_DOWN:
        case devices::LiftStateEnum::LIFT_UP:
        {
            if (liftState.ballWaitingSince > 0 && liftState.ballWaitingSince + 60000 < millis())
            {
                _liftLed->blink(360, 120); // Needs attention
            }
            else
            {
                _liftLed->set(true);
            }
            break;
        }
        }

        // Reset button timing state when not in LIFT_UP
        if (liftState.state != devices::LiftStateEnum::LIFT_UP)
        {
            _isBallStillLoaded = false;
            _liftButtonPressStartTime = 0;
        }

        // Lift Logic
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            // Init will start at press
            auto liftButtonState = _liftBtn->getState();
            if (liftButtonState.isPressed && liftButtonState.isPressedChanged)
            {
                playClickSound();
                _lift->init();
            }
            break;
        }

        case devices::LiftStateEnum::ERROR:
            // Handle error state - maybe blink LED faster

            // Play sound for new errors
            if (liftState.onErrorChange)
            {
                playErrorSound();
                blinkError(_liftLed);
            }
            break;

        // BUSY
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            // Busy, no new actions can be started
            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            auto liftButtonState = _liftBtn->getState();
            if (liftButtonState.isPressed && liftButtonState.isPressedChanged)
            {
                playClickSound();
                if (liftState.isLoaded)
                {
                    _lift->up();
                }
                else
                {
                    _lift->loadBall();
                }
            }

            break;
        }

        case devices::LiftStateEnum::LIFT_UP:
        {
            auto liftButtonState = _liftBtn->getState();
            if (liftButtonState.isPressed && liftButtonState.isPressedChanged)
            {
                if (liftState.isLoaded)
                {
                    // Loaded: start timing for unload duration
                    _liftButtonPressStartTime = millis();
                    _isBallStillLoaded = true;
                }
                else
                {
                    // Unloaded: move down
                    _lift->down();
                }
            }
            else if (_isBallStillLoaded && liftButtonState.isPressed)
            {
                // Button still pressed - check if we've reached the 500ms threshold
                unsigned long pressDuration = millis() - _liftButtonPressStartTime;
                if (pressDuration >= 500)
                {
                    // Long press: unload with full speed immediately
                    _lift->unloadBall(1.0f);
                    _buzzer->tune("Power Up:d=32,o=5,b=300:c,c#,d#,e,f#,g#,a#,b"); // Play power up tune
                    _isBallStillLoaded = false;
                }
            }
            else if (_isBallStillLoaded && !liftButtonState.isPressed)
            {
                // Slow unload
                _lift->unloadBall(0.5f);
                playClickSound();
                _isBallStillLoaded = false;
            }
            break;
        }
        }
    }

    void MarbleController::loopAutoLift()
    {
        // Auto lift control logic - automatic cycling through lift operations
        auto liftState = _lift->getState();

        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            // Wait for initialization
            _liftLed->set(false);
            _lift->init();
            break;

        case devices::LiftStateEnum::ERROR:
            // Play sound for new errors
            if (liftState.onErrorChange)
            {
                playErrorSound();
                blinkError(_liftLed);
            }
            break;

        // BUSY states - just blink LED
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN:
            _liftLed->blink(100, 100, 0);
            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            // Check if we need to wait before next operation
            if (_autoLiftDelayStart > 0 && (millis() - _autoLiftDelayStart) < _autoLiftDelayMs)
            {
                // Still waiting, do nothing
                break;
            }

            if (liftState.isLoaded)
            {
                // Loaded: move up to unload position
                _lift->up();
                _autoLiftDelayStart = 0; // Reset delay timer
            }
            else if (liftState.ballWaitingSince > 0)
            {
                // Not loaded: load a ball
                _lift->loadBall();
                _autoLiftDelayStart = 0; // Reset delay timer
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP:
        {
            // Check if we need to wait before next operation
            if (_autoLiftDelayStart > 0 && (millis() - _autoLiftDelayStart) < _autoLiftDelayMs)
            {
                // Still waiting, do nothing
                break;
            }

            if (liftState.isLoaded)
            {
                // Loaded: unload the ball
                _lift->unloadBall(1.0f); // Full unload
                _autoLiftDelayStart = 0; // Reset delay timer
            }
            else
            {
                // Not loaded: move down to loading position
                _lift->down();
                _autoLiftDelayStart = 0; // Reset delay timer
            }
            break;
        }
        }
    }

    void MarbleController::loopAutoWheel()
    {
        // Auto wheel control logic - similar to AutoMode.cpp
        auto wheelState = _wheel->getState();

        // Wheel led
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
        case devices::WheelStateEnum::IDLE:
            _wheelLed->set(true);
            break;

        case devices::WheelStateEnum::ERROR:
            blinkError(_wheelLed);
            break;

        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
        {
            _wheelLed->blink(240, 240); // LED blinks during init
            break;
        }
        case devices::WheelStateEnum::MOVING:
            _wheelLed->set(true);
            break;

        default:
            MLOG_WARN("%s: Unknown wheel state for led", toString().c_str());
            break;
        }

        // Trigger wheel logic
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            _wheel->init();
            break;

        case devices::WheelStateEnum::ERROR:
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::MOVING:
            break;

        case devices::WheelStateEnum::IDLE:
            // When idle, wait for random delay then trigger next breakpoint
            if (_wheelIdleStartTime == 0)
            {
                _wheelIdleStartTime = millis();
                _randomWheelDelayMs = 3000 + random(100, 20000);
                MLOG_INFO("%s: Next wheel trigger in %1.d ms", toString().c_str(), _randomWheelDelayMs / 100);
            }
            else if (millis() >= _wheelIdleStartTime + _randomWheelDelayMs)
            {
                MLOG_INFO("%s: Triggering wheel next breakpoint", toString().c_str());
                _wheel->nextBreakPoint();
                _wheelIdleStartTime = 0;
            }
            break;

        default:
            MLOG_WARN("%s: Unknown wheel state", toString().c_str());
            break;
        }
    }

    void MarbleController::loopManualWheel()
    {
        // Manual wheel control logic
        auto wheelState = _wheel->getState();
        auto wheelButtonState = _wheelBtn->getState();

        // Control wheel LED based on error state and movement
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            _wheelLed->set(true); // clickable init will start
            break;
        case devices::WheelStateEnum::ERROR:
            blinkError(_wheelLed);
            break;
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
            _wheelLed->blink(240, 240); // LED blinks during init
            break;
        case devices::WheelStateEnum::MOVING:
            _wheelLed->blink(480, 320, 160); // LED blinks when moving
            break;
        case devices::WheelStateEnum::IDLE:
            _wheelLed->set(true); // LED on when idle
            break;
        default:
            MLOG_ERROR("%s: Unknown wheel state: %d", toString().c_str(), static_cast<int>(wheelState.state));
            _wheelLed->set(false); // LED off for any other state
        }

        // Control wheel movement based on button state
        if (wheelButtonState.isPressed && wheelButtonState.isPressedChanged)
        {

            // Button just pressed - start continuous movement only if wheel is idle
            // Don't allow button usage when wheel is in error or init states
            if (wheelState.state == devices::WheelStateEnum::IDLE || wheelState.state == devices::WheelStateEnum::UNKNOWN || wheelState.state == devices::WheelStateEnum::MOVING)
            {
                MLOG_INFO("%s: Starting manual wheel movement as long button is pressed", toString().c_str());

                playClickSound();

                // Reset current position to prevent overflow;
                // if (wheelState.state != devices::WheelStateEnum::MOVING)
                // {
                // }
                _wheel->move(100000); // Large positive number for continuous movement
            }
            else if (wheelState.state == devices::WheelStateEnum::ERROR ||
                     wheelState.state == devices::WheelStateEnum::INIT)
            {
                MLOG_WARN("%s: Cannot start manual wheel movement - wheel is in %s state",
                          toString().c_str(), wheelState.state == devices::WheelStateEnum::ERROR ? "error" : "init");
            }
        }
        else if (!wheelButtonState.isPressed && wheelButtonState.isPressedChanged && wheelState.state == devices::WheelStateEnum::MOVING)
        {
            // Button released while moving - stop the wheel
            MLOG_INFO("%s: Stopping manual wheel movement", toString().c_str());

            playClickOffSound();

            _wheel->stop();
        }
    }

    void MarbleController::loopAutoSpiral()
    {
    }

    void MarbleController::loopManualSpiral()
    {
        auto spiralButtonState = _spiralBtn->getState();

        if (spiralButtonState.isPressed && spiralButtonState.isPressedChanged)
        {
            playClickSound();

            auto spiralLedState = _spiralLed->getState();
            if (spiralLedState.mode == "BLINKING")
            {
                _spiralLed->set(false);
            }
            else
            {
                _spiralLed->blink(20, 940);
            }
        }
    }

    void MarbleController::blinkError(Led *ledDevice)
    {
        if (ledDevice == nullptr)
        {
            return;
        }

        ledDevice->blink(20, 940);
    }

    void MarbleController::playStartupSound()
    {
        _buzzer->tune("Startup:d=4,o=6,b=1000:c,f,b#"); // Play error tune
    }

    void MarbleController::playErrorSound()
    {

        // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
        _buzzer->tune("Error:d=4,o=6,b=100:a,d"); // Play error tune
    }

    void MarbleController::playClickSound()
    {
        // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
        _buzzer->tone(640, 50);
    }

    void MarbleController::playClickOffSound()
    {
        // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
        _buzzer->tone(320, 50);
    }

} // namespace devices
