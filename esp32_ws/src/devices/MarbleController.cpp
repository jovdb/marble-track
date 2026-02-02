#include "devices/MarbleController.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Hv20tAudio.h"
#include "devices/Wheel.h"
#include "devices/Led.h"
#include "devices/Stepper.h"
#include "SongConstants.h"

extern DeviceManager deviceManager;

namespace devices
{

    MarbleController::MarbleController(const String &id) : Device(id, "marblecontroller")
    {
        _buzzer = new devices::Buzzer("buzzer");
        addChild(_buzzer);

        _audio = new devices::Hv20tAudio("hv20t");
        addChild(_audio);

        _lift = new devices::Lift("lift");
        addChild(_lift);

        _liftLed = new devices::Led("lift-led");
        addChild(_liftLed);

        _liftBtn = new devices::Button("lift-btn");
        addChild(_liftBtn);

        _manualButton = new devices::Button("manual-btn");
        addChild(_manualButton);

        // Create wheel with proper config
        _wheel = new devices::Wheel("wheel");
        addChild(_wheel);

        // Subscribe to wheel state changes
        _wheel->onStateChange([this](void *statePtr)
                              { this->onWheelStateChange(statePtr); });

        // Add wheel button LED
        _wheelLed = new devices::Led("wheel-led");
        addChild(_wheelLed);

        // Add wheel next button
        _wheelBtn = new devices::Button("wheel-btn");
        addChild(_wheelBtn);

        _spiralLed = new devices::Led("spiral-led");
        addChild(_spiralLed);

        _spiralBtn = new devices::Button("spiral-btn");
        addChild(_spiralBtn);
    }

    void MarbleController::setup()
    {
        Device::setup();
        playStartupSound();

        // Set auto mode based on manual button state during setup
        isAutoMode = !_manualButton->getState().isPressed;

        // Log the operating mode
        MLOG_INFO("%s Initialized in %s mode", toString().c_str(), isAutoMode ? "AUTO" : "MANUAL");

        if (isAutoMode)
        {
            _audio->play(songs::AUTO_MODE, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else
        {
            _audio->play(songs::MAN_MODE, devices::Hv20tPlayMode::QueueIfPlaying);
        }
    }

    void MarbleController::teardown()
    {
        Device::teardown();

        _liftButtonPressStartTime = 0;
        _isBallStillLoaded = false;
        _autoLiftDelayStart = 0;
        _wheelIdleStartTime = 0;
        _randomWheelDelayMs = 0;
        isAutoMode = false;
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
                // playClickSound();
                _audio->play(songs::getButtonClickSound(), devices::Hv20tPlayMode::SkipIfPlaying);
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
            _wheelLed->blink(480, 320, 160); // LED blinks when moving
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
                _randomWheelDelayMs = 3000 + random(100, 30000);
                MLOG_INFO("%s: Next wheel trigger in %.ds", toString().c_str(), _randomWheelDelayMs / 1000);
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

        // Control wheel movement based on button state
        auto wheelButtonState = _wheelBtn->getState();
        if (wheelButtonState.isPressed && wheelButtonState.isPressedChanged)
        {
            _audio->play(songs::FART);
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

                // playClickSound();
                _audio->play(songs::getButtonDownSound(), devices::Hv20tPlayMode::SkipIfPlaying);

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

            // playClickOffSound();
            // Only replace sound if down button is still playing
            auto index = _audio->getPlayingIndex();
            if (index == songs::getButtonDownSound())
            {
                _audio->play(songs::getButtonUpSound(), devices::Hv20tPlayMode::StopThenPlay);
            }
            else
            {
                _audio->play(songs::getButtonUpSound(), devices::Hv20tPlayMode::SkipIfPlaying);
            }

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
        //_buzzer->tune("Startup:d=4,o=6,b=1000:c,f,b#"); // Play error tune
        _audio->play(songs::STARTUP_SOUND);
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

    void MarbleController::onWheelStateChange(void *statePtr)
    {
        static devices::WheelStateEnum previousWheelState = devices::WheelStateEnum::UNKNOWN;

        auto *wheelState = static_cast<devices::WheelState *>(statePtr);
        if (!wheelState)
        {
            return;
        }

        // * -> CALIBRATING
        if (previousWheelState != devices::WheelStateEnum::CALIBRATING &&
            wheelState->state == devices::WheelStateEnum::CALIBRATING)
        {
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_START);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_END);
            _audio->play(songs::WHEEL_CALIBRATION_START, devices::Hv20tPlayMode::QueueIfPlaying);
        }

        // ERROR -> *
        if (previousWheelState == devices::WheelStateEnum::ERROR &&
            wheelState->state != devices::WheelStateEnum::ERROR)
        {
            // Don't play error that are not active anymore
            _audio->removeFromQueue(songs::WHEEL_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::CALIBRATION_FIRST_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::CALIBRATION_SECOND_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::UNEXPECTED_ZERO_TRIGGER);
        }

        // * -> ERROR
        if (previousWheelState != devices::WheelStateEnum::ERROR &&
            wheelState->state == devices::WheelStateEnum::ERROR)
        {

            if (wheelState->errorCode == devices::WheelErrorCode::CalibrationZeroNotFound)
            {
                _audio->play(songs::CALIBRATION_FIRST_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            else if (wheelState->errorCode == devices::WheelErrorCode::CalibrationSecondZeroNotFound)
            {
                _audio->play(songs::CALIBRATION_SECOND_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            else if (wheelState->errorCode == devices::WheelErrorCode::ZeroNotFound)
            {
                _audio->play(songs::WHEEL_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            else if (wheelState->errorCode == devices::WheelErrorCode::UnexpectedZeroTrigger)
            {
                _audio->play(songs::UNEXPECTED_ZERO_TRIGGER, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            else
            {
                MLOG_ERROR("%s: Unknown Wheel errorCode %d, cannot play audio", toString().c_str(), wheelState->errorCode);
            }
        }

        // CALIBRATING -> IDLE
        if (previousWheelState == devices::WheelStateEnum::CALIBRATING &&
            wheelState->state == devices::WheelStateEnum::IDLE)
        {
            _audio->play(songs::WHEEL_CALIBRATION_END, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        previousWheelState = wheelState->state;
    }

} // namespace devices
