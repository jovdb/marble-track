#include "devices/MarbleController.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"

extern DeviceManager deviceManager;

namespace devices
{

    MarbleController::MarbleController(const String &id) : Device(id, "MarbleController")
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
        _lift->setConfig(liftConfig);
        addChild(_lift);

        // Configure lift's child device pins
        auto liftStepper = _lift->getChildByIdAs<devices::Stepper>("lift-stepper");
        if (liftStepper)
        {
            auto liftStepperConfig = liftStepper->getConfig();
            liftStepperConfig.stepPin = 1;
            liftStepperConfig.dirPin = 2;
            liftStepperConfig.enablePin = 42;
            liftStepperConfig.invertEnable = true;
            liftStepper->setConfig(liftStepperConfig);
        }

        auto liftLimitSwitch = _lift->getChildByIdAs<devices::Button>("lift-limit");
        if (liftLimitSwitch)
        {
            auto liftLimitSwitchConfig = liftLimitSwitch->getConfig();
            liftLimitSwitchConfig.pin = 41;
            liftLimitSwitchConfig.pinMode = PinModeOption::PullUp;
            liftLimitSwitch->setConfig(liftLimitSwitchConfig);
        }

        auto liftBallSensor = _lift->getChildByIdAs<devices::Button>("lift-ball-sensor");
        if (liftBallSensor)
        {
            auto liftBallSensorConfig = liftBallSensor->getConfig();
            liftBallSensorConfig.pin = 40;
            liftBallSensorConfig.pinMode = PinModeOption::PullUp;
            liftBallSensor->setConfig(liftBallSensorConfig);
        }

        auto liftLoader = _lift->getChildByIdAs<devices::Servo>("lift-loader");
        if (liftLoader)
        {
            auto liftLoaderConfig = liftLoader->getConfig();
            liftLoaderConfig.pin = 39;
            liftLoaderConfig.mcpwmChannel = -1;
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
            liftUnloaderConfig.mcpwmChannel = -1;
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
        liftLedConfig.pin = 45;
        _liftLed->setConfig(liftLedConfig);
        addChild(_liftLed);

        _liftButton = new devices::Button("lift-button");
        auto liftButtonConfig = _liftButton->getConfig();
        liftButtonConfig.name = "Lift Button";
        liftButtonConfig.pin = 48;
        liftButtonConfig.pinMode = PinModeOption::PullUp;
        _liftButton->setConfig(liftButtonConfig);
        addChild(_liftButton);

        _manualButton = new devices::Button("manual-btn");
        auto manualButtonConfig = _manualButton->getConfig();
        manualButtonConfig.name = "Manual Mode Button";
        manualButtonConfig.pin = 12;
        manualButtonConfig.pinMode = PinModeOption::PullUp;
        _manualButton->setConfig(manualButtonConfig);
        addChild(_manualButton);
    }

    void MarbleController::setup()
    {
        Device::setup();

        // Set auto mode based on manual button state during setup
        isAutoMode = !_manualButton->getState().isPressed;

        // Log the operating mode
        MLOG_INFO("MarbleController initialized in %s mode", isAutoMode ? "AUTO" : "MANUAL");
    }

    void MarbleController::loop()
    {
        Device::loop();

        if (isAutoMode)
        {
            loopAutoLift();
        }
        else
        {
            loopManualLift();
        }
    }

    void MarbleController::loopManualLift()
    {
        // Lift control logic

        auto liftState = _lift->getState();

        // Reset button timing state when not in LIFT_UP
        if (liftState.state != devices::LiftStateEnum::LIFT_UP)
        {
            _waitingForLiftButtonRelease = false;
            _liftButtonPressStartTime = 0;
        }

        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            // Wait for initialization
            _liftLed->set(false);
            _lift->init();
            break;

        case devices::LiftStateEnum::ERROR:
            // Handle error state - maybe blink LED faster
            _liftLed->set(false);

            // Play sound for new errors
            if (liftState.onErrorChange)
            {
                playErrorSound();
            }
            break;

        // BUSY
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            _liftLed->blink(500, 500);
            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            _liftLed->set(true);
            auto liftButtonState = _liftButton->getState();
            if (liftButtonState.isPressed && liftButtonState.isPressedChanged)
            {
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
            _liftLed->set(true);
            auto liftButtonState = _liftButton->getState();
            if (liftButtonState.isPressed && liftButtonState.isPressedChanged)
            {
                if (liftState.isLoaded)
                {
                    // Loaded: start timing for unload duration
                    if (!_waitingForLiftButtonRelease)
                    {
                        _liftButtonPressStartTime = millis();
                        _waitingForLiftButtonRelease = true;
                    }
                }
                else
                {
                    // Unloaded: move down
                    _lift->down();
                }
            }
            else if (_waitingForLiftButtonRelease && !liftButtonState.isPressed)
            {
                // Button released after timing for unload
                unsigned long pressDuration = millis() - _liftButtonPressStartTime;
                float durationRatio = (pressDuration >= 500) ? 1.0f : 0.6f;
                _lift->unloadBall(durationRatio);

                if (pressDuration >= 500)
                {
                    _buzzer->tune("Power Up:d=32,o=5,b=300:c,c#,d#,e,f#,g#,a#,b"); // Play error tune
                }
                // Reset timing state
                _waitingForLiftButtonRelease = false;
                _liftButtonPressStartTime = 0;
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
            // Handle error state - maybe blink LED faster
            _liftLed->set(false);

            // Play sound for new errors
            if (liftState.onErrorChange)
            {
                playErrorSound();
            }
            break;

        // BUSY states - just blink LED
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN:
            _liftLed->blink(500, 500);
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
            else if (liftState.isBallWaiting)
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

    void MarbleController::playErrorSound()
    {
        if (_buzzer)
        {
            // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
            _buzzer->tune("Error:d=4,o=4,b=100:a,d"); // Play error tune
        }
    }

} // namespace devices
