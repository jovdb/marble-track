#include "devices/MarbleController.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Hv20tAudio.h"
#include "devices/Wheel.h"
#include "devices/Led.h"
#include "devices/Stepper.h"
#include "devices/ServoGate.h"
#include "SongConstants.h"

extern DeviceManager deviceManager;

namespace devices
{

    namespace lift_timing
    {
        static constexpr unsigned long PowerSongDurationMs = 5200UL;
        static constexpr unsigned long PowerSongStartDelayMs = 500UL;
        static constexpr unsigned long AutoPowerSongStartDelayMs = 1000UL;
        static constexpr unsigned long AutoNoBallRandomMinDelayMs = 120000UL;
        static constexpr unsigned long AutoNoBallRandomMaxDelayMs = 300000UL;
        static constexpr unsigned long ErrorLongPressDurationMs = 8000UL; // 10 seconds for error recovery
        static constexpr float AutoDownNoBallSpeedRatio = 0.2f;
        static constexpr float AutoDownNormalSpeedRatio = 1.0f;
        static constexpr float LiftAutoSpeedRatio = 0.25f;
        static constexpr float LiftManualSpeedRatio = 1.0f;
    }

    namespace wheel_timing
    {
        static constexpr float AutoSpeedRatio = 0.8f;
    }

    MarbleController::MarbleController(const String &id) : Device(id, "marblecontroller")
    {
        _buzzer = new devices::Buzzer("buzzer");
        addChild(_buzzer);

        _audio = new devices::Hv20tAudio("hv20t");
        addChild(_audio);

        _lift = new devices::Lift("lift");
        addChild(_lift);

        // Subscribe to lift state changes
        _lift->onStateChange([this](void *statePtr)
                             { this->onLiftStateChange(statePtr); });

        _liftLed = new devices::Led("lift-led");
        addChild(_liftLed);

        _liftBtn = new devices::Button("lift-btn");
        addChild(_liftBtn);

        _manualButton = new devices::Button("manual-btn");
        addChild(_manualButton);

        auto *tower = new devices::ServoGate("tower");

        auto towerConfig = tower->getConfig();
        towerConfig.name = "ServoGate";
        towerConfig.openDelayMs = 500;
        towerConfig.closeDelayMs = 1000;
        towerConfig.betweenDelayMs = 500;
        towerConfig.fullQueueCount = 8;
        tower->setConfig(towerConfig);

        for (Device *child : tower->getChildren())
        {
            if (!child)
            {
                continue;
            }

            if (child->getId() == "tower-button" && child->getType() == "button")
            {
                auto *button = static_cast<devices::Button *>(child);
                auto buttonConfig = button->getConfig();
                buttonConfig.pinConfig.pin = 17;
                buttonConfig.pinConfig.expanderId = "";
                buttonConfig.name = "ServoGate Button";
                buttonConfig.debounceTimeInMs = 150;
                buttonConfig.pinMode = devices::PinModeOption::PullDown;
                buttonConfig.buttonType = devices::ButtonType::NormalOpen;
                button->setConfig(buttonConfig);
            }
            else if (child->getId() == "tower-servo" && child->getType() == "servo")
            {
                auto *servo = static_cast<devices::Servo *>(child);
                auto servoConfig = servo->getConfig();
                servoConfig.pinConfig.pin = 0;
                servoConfig.pinConfig.expanderId = "pwm-ex-1";
                servoConfig.name = "ServoGate Servo";
                servoConfig.mcpwmChannel = -1;
                servoConfig.frequency = 50;
                servoConfig.resolutionBits = 10;
                servoConfig.minDutyCycle = 11;
                servoConfig.maxDutyCycle = 4;
                servoConfig.defaultDurationInMs = 4000;
                servo->setConfig(servoConfig);
            }
        }

        addChild(tower);

        // Create wheel with proper config
        _wheel = new devices::Wheel("wheel");
        addChild(_wheel);

        JsonDocument splitterConfig;
        splitterConfig["name"] = "Splitter";

        _splitter = new devices::Wheel("splitter");
        _splitter->jsonToConfig(splitterConfig);
        addChild(_splitter);

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

        _splitterSensor = new devices::Button("splitter-sensor");
        addChild(_splitterSensor);
    }

    void MarbleController::setup()
    {
        Device::setup();

        playStartupSound();

        // Initialize idle tracking
        _lastButtonPressTime = millis();
        _idleSoundPlayed = false;
        _liftQueuedPresses = 0;
        _autoPowerUnloadPending = false;
        _autoPowerUnloadSongStarted = false;
        _autoPowerUnloadStartTime = 0;
        _autoLiftUpLoadedSince = 0;
        _autoNoBallLiftStartTime = 0;
        _autoNoBallLiftDelayMs = 0;
        _autoLiftMovingDownSlow = false;

        // Initialize splitter sensor variables
        _splitterCounter = 0;
        _splitterDelayStart = 0;
        _splitterSensorPressStartTime = 0;
        _splitterSensorWasPressed = false;
        _splitterLongPressApplied = false;
        _splitterMovePending = false;
        _splitterMoveSawBusy = false;
        _autoLiftMovingDownSlow = false;

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
        _isLiftPowerUnloadSongPlaying = false;
        _liftQueuedPresses = 0;
        _autoPowerUnloadPending = false;
        _autoPowerUnloadSongStarted = false;
        _autoPowerUnloadStartTime = 0;
        _autoLiftUpLoadedSince = 0;
        _autoNoBallLiftStartTime = 0;
        _autoNoBallLiftDelayMs = 0;
        _autoLiftMovingDownSlow = false;
        _autoLiftDelayStart = 0;
        _wheelIdleStartTime = 0;
        _randomWheelDelayMs = 0;
        _lastButtonPressTime = 0;
        _idleSoundPlayed = false;
        isAutoMode = false;

        // Reset splitter sensor variables
        _splitterCounter = 0;
        _splitterDelayStart = 0;
        _splitterSensorPressStartTime = 0;
        _splitterSensorWasPressed = false;
        _splitterLongPressApplied = false;
        _splitterMovePending = false;
        _splitterMoveSawBusy = false;
    }

    void MarbleController::loop()
    {
        Device::loop();

        // Check for idle timeout (5 minutes = 300000 ms)
        if (!isAutoMode && millis() - _lastButtonPressTime > 300000UL && !_idleSoundPlayed)
        {
            _audio->play(songs::NOTIFICATION, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::IDLE, devices::Hv20tPlayMode::QueueIfPlaying);
            _idleSoundPlayed = true;
        }

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

        loopSplitter();
    }

    void MarbleController::loopManualLift()
    {
        auto liftState = _lift->getState();
        auto liftButtonState = _liftBtn->getState();
        const bool liftButtonPressedEdge = liftButtonState.isPressed && liftButtonState.isPressedChanged;
        const bool liftButtonReleasedEdge = !liftButtonState.isPressed && liftButtonState.isPressedChanged;

        // Idle tracking
        if (liftButtonPressedEdge)
        {
            _lastButtonPressTime = millis();
            _idleSoundPlayed = false;
        }

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
            blinkInit(_liftLed);
            break;
        }
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
            blinkBusy(_liftLed);
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            if (_autoLiftMovingDownSlow)
            {
                _liftLed->set(true);
            }
            else
            {
                blinkBusy(_liftLed);
            }
            break;
        case devices::LiftStateEnum::UNKNOWN:
        case devices::LiftStateEnum::LIFT_DOWN:
        case devices::LiftStateEnum::LIFT_UP:
        {
            if (liftState.ballWaitingSince > 0 && liftState.ballWaitingSince + 60000 < millis())
            {
                blinkAttention(_liftLed);
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
            // _liftButtonPressStartTime = 0;
            _isLiftPowerUnloadSongPlaying = false;
        }

        // Lift Logic
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            _liftQueuedPresses = 0;
            // Init will start at press
            if (liftButtonPressedEdge)
            {
                _lift->init(lift_timing::LiftManualSpeedRatio);
                playButtonClick();
            }
            break;
        }

        case devices::LiftStateEnum::ERROR:
        {
            _liftQueuedPresses = 0;

            if (liftButtonPressedEdge)
            {
                playLiftError(_lift->getErrorCode());
                _liftButtonPressStartTime = millis();
            }

            unsigned long pressDuration = _liftButtonPressStartTime > 0 ? millis() - _liftButtonPressStartTime : 0;

            // Check for long press while button is held
            if (liftButtonState.isPressed && (pressDuration >= lift_timing::ErrorLongPressDurationMs))
            {
                MLOG_INFO("%s: Error recovery long press detected, starting lift init", toString().c_str());
                _lift->init(lift_timing::LiftManualSpeedRatio);
                playButtonClick();
                _liftButtonPressStartTime = 0; // Reset to prevent retriggering
            }
            break;
        }
        case devices::LiftStateEnum::INIT:
            if (liftButtonPressedEdge)
            {
                playErrorSound(devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::LIFT_INIT_BUSY, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
            if (liftButtonPressedEdge)
            {
                if (_liftQueuedPresses < 3)
                {
                    _liftQueuedPresses++;
                    playButtonClick({songs::LIFT_STOP});
                }
                else
                {
                    playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                }
            }
            break;
        case devices::LiftStateEnum::MOVING_UP:
            if (liftButtonPressedEdge)
            {
                if (_liftQueuedPresses < 2)
                {
                    _liftQueuedPresses++;
                    playButtonClick({songs::LIFT_STOP});
                }
                else
                {
                    playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                }
            }
            break;
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
            if (liftButtonPressedEdge)
            {
                if (_liftQueuedPresses < 1)
                {
                    _liftQueuedPresses++;
                    playButtonClick({songs::LIFT_STOP});
                }
                else
                {
                    playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                }
            }
            break;
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            if (liftButtonPressedEdge)
            {
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            // Replay queued press: loaded + queued => go up and consume one
            if (_liftQueuedPresses > 0 || liftButtonPressedEdge)
            {
                if (liftState.isLoaded)
                {
                    if (_lift->up(lift_timing::LiftManualSpeedRatio))
                    {
                        if (!liftButtonPressedEdge)
                            _liftQueuedPresses--;
                    }
                    if (liftButtonPressedEdge)
                        playButtonClick({songs::LIFT_STOP});
                }
                else
                {
                    if (_lift->loadBall())
                    {
                        if (!liftButtonPressedEdge)
                            _liftQueuedPresses--;
                    }
                    if (liftButtonPressedEdge)
                        playButtonClick({songs::LIFT_STOP});
                }
            }
            break;
        }
        case devices::LiftStateEnum::LIFT_UP:
        {
            // Queued
            if (_liftQueuedPresses > 0 && !liftButtonPressedEdge)
            {
                if (liftState.isLoaded)
                {
                    if (_lift->unloadBall(1.0f))
                    {
                        _liftQueuedPresses--;
                        _isBallStillLoaded = false;
                        _liftButtonPressStartTime = 0;
                    }
                }
                else
                {
                    // If not loaded but still queued, try going down to load if possible
                    if (_lift->down(lift_timing::LiftManualSpeedRatio))
                    {
                        _liftQueuedPresses = 0;
                    }
                }
            }

            if (liftButtonPressedEdge)
            {
                if (liftState.isLoaded)
                {
                    // Loaded: start timing for unload duration
                    _liftButtonPressStartTime = millis();
                    _isBallStillLoaded = true;
                    _isLiftPowerUnloadSongPlaying = false;
                }
                else
                {
                    // Unloaded: move down
                    _lift->down(lift_timing::LiftManualSpeedRatio);
                    playButtonClick({songs::LIFT_STOP});
                }
            }
            else if (_isBallStillLoaded && liftButtonState.isPressed)
            {
                // Button still pressed - play power unload song from 500ms
                unsigned long pressDuration = millis() - _liftButtonPressStartTime;

                if (pressDuration >= lift_timing::PowerSongStartDelayMs && !_isLiftPowerUnloadSongPlaying)
                {
                    _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
                    _isLiftPowerUnloadSongPlaying = true;
                }

                if (pressDuration >= lift_timing::PowerSongDurationMs)
                {
                    MLOG_INFO("%s: Long press detected (%.2fs), Power unload", toString().c_str());
                    // Long press: unload with full speed immediately
                    _lift->unloadBall(0.2f);
                    _isBallStillLoaded = false;
                }
            }
            else if (_isBallStillLoaded && !liftButtonState.isPressed)
            {
                unsigned long pressDuration = millis() - _liftButtonPressStartTime;
                if (pressDuration < lift_timing::PowerSongDurationMs && _isLiftPowerUnloadSongPlaying)
                {
                    _audio->stop();
                }
                _isLiftPowerUnloadSongPlaying = false;

                // Normal click unload (default duration)
                _lift->unloadBall(1.0f);
                // playClickSound();
                playButtonClick({songs::LIFT_STOP});
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
        auto liftButtonState = _liftBtn->getState();
        const bool liftButtonPressedEdge = liftButtonState.isPressed && liftButtonState.isPressedChanged;
        const bool liftButtonReleasedEdge = !liftButtonState.isPressed && liftButtonState.isPressedChanged;

        if (liftState.state != devices::LiftStateEnum::LIFT_UP || !liftState.isLoaded)
        {
            _autoPowerUnloadPending = false;
            _autoPowerUnloadSongStarted = false;
            _autoPowerUnloadStartTime = 0;
            _autoLiftUpLoadedSince = 0;
        }

        if (liftState.state != devices::LiftStateEnum::LIFT_DOWN || liftState.isLoaded || liftState.ballWaitingSince > 0)
        {
            _autoNoBallLiftStartTime = 0;
            _autoNoBallLiftDelayMs = 0;
        }

        if (liftState.state != devices::LiftStateEnum::MOVING_DOWN)
        {
            _autoLiftMovingDownSlow = false;
        }

        // LED
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            _liftLed->set(false);
            break;
        case devices::LiftStateEnum::ERROR:
            blinkError(_liftLed);
            break;
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::LIFT_UP:
            blinkBusy(_liftLed);
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            if (_autoLiftMovingDownSlow)
            {
                _liftLed->set(true);
            }
            else
            {
                blinkBusy(_liftLed);
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            _liftLed->set(true);
            break;
        }
        }

        // LOGIC
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            _lift->init(lift_timing::LiftAutoSpeedRatio);
            break;

        case devices::LiftStateEnum::ERROR:
            if (liftButtonPressedEdge)
            {
                playLiftError(_lift->getErrorCode());
                _liftButtonPressStartTime = millis();
            }

            // Check for long press while button is held
            if (liftButtonState.isPressed && _liftButtonPressStartTime > 0 &&
                ((millis() - _liftButtonPressStartTime) >= lift_timing::ErrorLongPressDurationMs))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting lift init", toString().c_str());
                _lift->init(lift_timing::LiftAutoSpeedRatio);
                playButtonClick();
                _liftButtonPressStartTime = 0; // Reset to prevent retriggering
            }
            break;

        // BUSY states - just blink LED
        case devices::LiftStateEnum::INIT:
            if (liftButtonPressedEdge)
            {
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                _audio->play(songs::LIFT_INIT_BUSY, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
            if (liftButtonPressedEdge)
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
            break;

        case devices::LiftStateEnum::MOVING_DOWN:

            if (_autoLiftMovingDownSlow && liftState.ballWaitingSince > 0)
            {
                if (_lift->down(lift_timing::AutoDownNormalSpeedRatio * lift_timing::LiftAutoSpeedRatio))
                {
                    _autoLiftMovingDownSlow = false;
                    MLOG_INFO("%s: Ball waiting detected during auto down, switching to normal speed", toString().c_str());
                }
            }
            else if (liftButtonPressedEdge)
            {
                if (_autoLiftMovingDownSlow)
                {
                    if (_lift->down(lift_timing::AutoDownNormalSpeedRatio * lift_timing::LiftAutoSpeedRatio))
                    {
                        _autoLiftMovingDownSlow = false;
                        playButtonClick({songs::LIFT_STOP});
                    }
                }
                else
                {
                    playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                }
            }

            break;

        case devices::LiftStateEnum::LIFT_DOWN:
        {
            _isLiftPowerUnloadSongPlaying = false;

            if (liftState.isLoaded)
            {
                // Loaded: move up to unload position
                _lift->up(lift_timing::LiftAutoSpeedRatio);
                _autoLiftDelayStart = 0; // Reset delay timer
            }
            else if (liftState.ballWaitingSince > 0)
            {
                _autoNoBallLiftStartTime = 0;
                _autoNoBallLiftDelayMs = 0;

                // Not loaded: wait 1000ms before starting load
                if (_autoLiftDelayStart == 0)
                {
                    _autoLiftDelayStart = millis();
                    break;
                }

                if ((millis() - _autoLiftDelayStart) < _autoLiftDelayMs)
                {
                    break;
                }

                _lift->loadBall();
                _autoLiftDelayStart = 0;
            }
            else
            {
                _autoLiftDelayStart = 0;

                if (liftButtonPressedEdge)
                {
                    playButtonDown({songs::LIFT_STOP});
                }

                if (liftButtonReleasedEdge)
                {
                    playButtonUp({songs::LIFT_STOP});
                    _autoNoBallLiftStartTime = 0;
                    _autoNoBallLiftDelayMs = 0;
                    _lift->loadBall();
                    break;
                }

                if (_autoNoBallLiftStartTime == 0)
                {
                    _autoNoBallLiftStartTime = millis();
                    _autoNoBallLiftDelayMs = random(
                        lift_timing::AutoNoBallRandomMinDelayMs,
                        lift_timing::AutoNoBallRandomMaxDelayMs + 1UL);
                    break;
                }

                if ((millis() - _autoNoBallLiftStartTime) >= _autoNoBallLiftDelayMs)
                {
                    MLOG_INFO("%s: Auto lift random start (no ball waiting) after %lus",
                              toString().c_str(),
                              _autoNoBallLiftDelayMs / 1000UL);

                    if (_lift->loadBall())
                    {
                        _autoNoBallLiftStartTime = 0;
                        _autoNoBallLiftDelayMs = 0;
                    }
                }
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP:
        {
            if (liftButtonPressedEdge)
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});

            // Check if we need to wait before next operation
            if (_autoLiftDelayStart > 0 && (millis() - _autoLiftDelayStart) < _autoLiftDelayMs)
            {
                // Still waiting, do nothing
                break;
            }

            if (liftState.isLoaded)
            {
                if (_autoLiftUpLoadedSince == 0)
                {
                    _autoLiftUpLoadedSince = millis();
                    _autoPowerUnloadPending = (random(100) < 25);
                    _autoPowerUnloadSongStarted = false;
                    _autoPowerUnloadStartTime = 0;
                    break;
                }

                const unsigned long loadedLiftUpElapsed = millis() - _autoLiftUpLoadedSince;

                // Wait until lift-end song is ready (loaded LIFT_UP + 1000ms)
                if (loadedLiftUpElapsed < lift_timing::AutoPowerSongStartDelayMs)
                {
                    break;
                }

                if (_autoPowerUnloadPending)
                {
                    if (!_autoPowerUnloadSongStarted)
                    {
                        _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
                        _autoPowerUnloadSongStarted = true;
                        _autoPowerUnloadStartTime = millis();
                        break;
                    }

                    const unsigned long powerSongElapsed = millis() - _autoPowerUnloadStartTime;
                    if (powerSongElapsed >= lift_timing::PowerSongDurationMs - 500)
                    {
                        if (_lift->unloadBall(0.2f))
                        {
                            _autoPowerUnloadPending = false;
                            _autoPowerUnloadSongStarted = false;
                            _autoPowerUnloadStartTime = 0;
                            _autoLiftUpLoadedSince = 0;
                            _autoLiftDelayStart = 0;
                        }
                    }
                }
                else
                {
                    // Non power unload: also wait 1000ms at loaded LIFT_UP, then unload normally
                    if (_lift->unloadBall(1.0f))
                    {
                        _autoLiftUpLoadedSince = 0;
                        _autoLiftDelayStart = 0;
                    }
                }
            }
            else
            {
                // Not loaded: move down to loading position
                if (liftState.ballWaitingSince > 0)
                {
                    _autoLiftMovingDownSlow = false;
                    _lift->down(lift_timing::AutoDownNormalSpeedRatio * lift_timing::LiftAutoSpeedRatio);
                }
                else
                {
                    if (_lift->down(lift_timing::AutoDownNoBallSpeedRatio * lift_timing::LiftAutoSpeedRatio))
                    {
                        _autoLiftMovingDownSlow = true;
                    }
                }
                _autoLiftDelayStart = 0; // Reset delay timer
                _autoPowerUnloadPending = false;
                _autoPowerUnloadSongStarted = false;
                _autoPowerUnloadStartTime = 0;
                _autoLiftUpLoadedSince = 0;
            }
            break;
        }
        }

        // Idle tracking
        if (liftButtonPressedEdge)
        {
            _lastButtonPressTime = millis();
            _idleSoundPlayed = false;
        }
    }

    void MarbleController::loopAutoWheel()
    {
        // Auto wheel control logic - similar to AutoMode.cpp
        auto wheelState = _wheel->getState();
        auto wheelButtonState = _wheelBtn->getState();
        auto isWheelButtonPressedEdge = wheelButtonState.isPressed && wheelButtonState.isPressedChanged;

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
            blinkInit(_wheelLed);
            break;
        }
        case devices::WheelStateEnum::MOVING:
            blinkBusy(_wheelLed);
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
            if (isWheelButtonPressedEdge)
            {
                playWheelError(_wheel->getErrorCode());
                _wheelButtonPressStartTime = millis();
                _wheelButtonLongPressTriggered = false;
            }
            // Check for long press while button is held (8s → init)
            if (wheelButtonState.isPressed && _wheelButtonPressStartTime > 0 && !_wheelButtonLongPressTriggered &&
                ((millis() - _wheelButtonPressStartTime) >= WHEEL_LONG_PRESS_DURATION_MS))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting wheel init", toString().c_str());
                _wheel->init();
                playButtonClick();
                _wheelButtonPressStartTime = 0;
                _wheelButtonLongPressTriggered = true;
            }
            if (!wheelButtonState.isPressed)
            {
                _wheelButtonPressStartTime = 0;
                _wheelButtonLongPressTriggered = false;
            }
            break;

        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::MOVING:
            if (isWheelButtonPressedEdge)
            {
                playErrorSound();
            }
            break;

        case devices::WheelStateEnum::IDLE:
            // When idle, wait for random delay then trigger next breakpoint
            if (_wheelIdleStartTime == 0)
            {
                _wheelIdleStartTime = millis();
                _randomWheelDelayMs = 3000 + random(100, 30000);
                MLOG_INFO("%s: Next random wheel trigger starts in %.ds", toString().c_str(), _randomWheelDelayMs / 1000);
            }
            else if (millis() >= _wheelIdleStartTime + _randomWheelDelayMs)
            {
                MLOG_INFO("%s: Triggering wheel next breakpoint", toString().c_str());
                _wheel->nextBreakPoint(wheel_timing::AutoSpeedRatio);
                _wheelIdleStartTime = 0;
            }
            else if (isWheelButtonPressedEdge)
            {
                _wheel->nextBreakPoint(wheel_timing::AutoSpeedRatio);
                playButtonClick();
            }

            break;

        default:
            MLOG_WARN("%s: Unknown wheel state", toString().c_str());
            break;
        }

        // Control wheel movement based on button state
        if (isWheelButtonPressedEdge)
        {
            // Reset idle timer
            _lastButtonPressTime = millis();
            _idleSoundPlayed = false;
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
            blinkInit(_wheelLed);
            break;
        case devices::WheelStateEnum::MOVING:
            blinkBusy(_wheelLed);
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
            // Reset idle timer
            _lastButtonPressTime = millis();
            _idleSoundPlayed = false;
            // Button just pressed - start timing for long press detection
            _wheelButtonPressStartTime = millis();
            _wheelButtonLongPressTriggered = false;

            // Button just pressed - start continuous movement only if wheel is idle
            // Don't allow button usage when wheel is in error or init states
            if (wheelState.state == devices::WheelStateEnum::IDLE || wheelState.state == devices::WheelStateEnum::UNKNOWN || wheelState.state == devices::WheelStateEnum::MOVING || wheelState.state == devices::WheelStateEnum::INIT)
            {
                MLOG_INFO("%s: Starting manual wheel movement as long button is pressed", toString().c_str());

                // playClickSound();
                playButtonDown();

                // Reset current position to prevent overflow;
                // if (wheelState.state != devices::WheelStateEnum::MOVING)
                // {
                // }
                _wheel->move(100000); // Large positive number for continuous movement
            }
            else if (wheelState.state == devices::WheelStateEnum::ERROR)
            {
                // In error state: play error sound and let the long-press timer run
                playWheelError(_wheel->getErrorCode());
            }
        }
        else if (wheelButtonState.isPressed && !_wheelButtonLongPressTriggered)
        {
            // Button still pressed - check for long press (8 seconds)
            unsigned long pressDuration = millis() - _wheelButtonPressStartTime;
            if (pressDuration >= WHEEL_LONG_PRESS_DURATION_MS)
            {
                if (wheelState.state == devices::WheelStateEnum::ERROR)
                {
                    // Error recovery: long press starts init
                    MLOG_INFO("%s: Error recovery long press detected, starting wheel init", toString().c_str());
                    _wheel->init();
                    playButtonClick();
                }
                else
                {
                    // Long press detected - trigger next breakpoint
                    MLOG_INFO("%s: Long wheel press detected - triggering next breakpoint", toString().c_str());
                    _wheel->stop(); // Stop continuous movement before triggering next breakpoint
                    _wheel->nextBreakPoint();
                    _audio->play(songs::WHEEL_GOTO_BREAKPOINT, devices::Hv20tPlayMode::SkipIfPlaying);
                }
                _wheelButtonLongPressTriggered = true;
            }
        }
        else if (!wheelButtonState.isPressed && wheelButtonState.isPressedChanged && wheelState.state == devices::WheelStateEnum::MOVING)
        {

            if (_wheelButtonLongPressTriggered)
            {
                // Reset long press tracking
                _wheelButtonPressStartTime = 0;
                _wheelButtonLongPressTriggered = false;
                return;
            }

            // Button released while moving - stop the wheel
            MLOG_INFO("%s: Stopping manual wheel movement", toString().c_str());

            // Reset long press tracking
            _wheelButtonPressStartTime = 0;
            _wheelButtonLongPressTriggered = false;

            // playClickOffSound();
            playButtonUp();

            _wheel->stop();
        }
        else if (!wheelButtonState.isPressed && wheelButtonState.isPressedChanged)
        {
            // Button released when not moving - just reset tracking
            _wheelButtonPressStartTime = 0;
            _wheelButtonLongPressTriggered = false;
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
                // _spiralLed->blink(20, 940);
            }
        }
    }

    void MarbleController::loopSplitter()
    {
        if (!_splitterSensor || !_splitter)
        {
            return;
        }

        const unsigned long now = millis();
        const auto splitterSensorState = _splitterSensor->getState();
        const bool isPressed = splitterSensorState.isPressed;
        const bool pressedEdge = isPressed && !_splitterSensorWasPressed;
        const bool releasedEdge = !isPressed && _splitterSensorWasPressed;
        _splitterSensorWasPressed = isPressed;

        if (pressedEdge)
        {
            _splitterSensorPressStartTime = now;
            _splitterLongPressApplied = false;

            if (_splitterCounter < 5)
            {
                _splitterCounter++;
            }

            MLOG_INFO("%s: Splitter pulse queued, counter=%u", toString().c_str(), static_cast<unsigned>(_splitterCounter));
        }

        if (releasedEdge)
        {
            _splitterSensorPressStartTime = 0;
            _splitterLongPressApplied = false;
        }

        if (isPressed && _splitterSensorPressStartTime > 0 && !_splitterLongPressApplied)
        {
            const unsigned long pressDuration = now - _splitterSensorPressStartTime;
            if (pressDuration > 1000)
            {
                if (_splitterCounter < 3)
                {
                    _splitterCounter = 3;
                }
                _splitterLongPressApplied = true;
                MLOG_INFO("%s: Splitter long press detected, counter=%u", toString().c_str(), static_cast<unsigned>(_splitterCounter));
            }
        }

        auto splitterState = _splitter->getState();

        // Wait for motion to finish before consuming a queued pulse.
        if (_splitterMovePending)
        {
            if (splitterState.state != devices::WheelStateEnum::IDLE)
            {
                _splitterMoveSawBusy = true;
            }
            else if (_splitterMoveSawBusy)
            {
                _splitterMovePending = false;
                _splitterMoveSawBusy = false;

                if (_splitterCounter > 0)
                {
                    _splitterCounter--;
                }

                MLOG_INFO("%s: Splitter reached idle, remaining queue=%u", toString().c_str(), static_cast<unsigned>(_splitterCounter));
                _splitterDelayStart = (_splitterCounter > 0) ? now : 0;
            }

            return;
        }

        if (_splitterCounter == 0)
        {
            _splitterDelayStart = 0;
            return;
        }

        if (_splitterDelayStart == 0)
        {
            _splitterDelayStart = now;
            MLOG_INFO("%s: Splitter delay started, queue=%u", toString().c_str(), static_cast<unsigned>(_splitterCounter));
            return;
        }

        if ((now - _splitterDelayStart) < 500UL)
        {
            return;
        }

        if (_splitter->nextBreakPoint())
        {
            _splitterMovePending = true;
            splitterState = _splitter->getState();
            _splitterMoveSawBusy = (splitterState.state != devices::WheelStateEnum::IDLE);
            _splitterDelayStart = 0;
            MLOG_INFO("%s: Splitter move started, queue=%u", toString().c_str(), static_cast<unsigned>(_splitterCounter));
            return;
        }

        // Retry later if the command was rejected (for example while not ready).
        _splitterDelayStart = now;
    }

    void MarbleController::blinkError(Led *ledDevice)
    {
        if (!ledDevice)
        {
            return;
        }

        ledDevice->blink(20, 940);
    }

    void MarbleController::blinkBusy(Led *ledDevice)
    {
        if (!ledDevice)
        {
            return;
        }

        ledDevice->blink(480, 480);
    }

    void MarbleController::blinkInit(Led *ledDevice)
    {
        if (!ledDevice)
        {
            return;
        }

        ledDevice->blink(480, 480);
    }

    void MarbleController::blinkAttention(Led *ledDevice)
    {
        if (!ledDevice)
        {
            return;
        }

        ledDevice->blink(360, 120); // Needs attention
    }

    void MarbleController::playStartupSound()
    {
        //_buzzer->tune("Startup:d=4,o=6,b=1000:c,f,b#"); // Play error tune
        _audio->play(songs::STARTUP_SOUND, devices::Hv20tPlayMode::QueueIfPlaying);
    }

    void MarbleController::playErrorSound(Hv20tPlayMode mode, std::vector<int> additionalReplaceSongIndexes)
    {

        // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
        // _buzzer->tune("Error:d=4,o=6,b=100:a,d"); // Play error tune

        // Create the default replace list with button sounds
        std::vector<int> replaceSongIndexes = {songs::getButtonDownSound(), songs::getButtonUpSound(), songs::getButtonClickSound(), songs::ERROR};

        // Add any additional indexes
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());

        // Check if any song from the replace list is currently playing
        auto currentIndex = _audio->getPlayingIndex();
        bool shouldReplace = false;

        for (int songIndex : replaceSongIndexes)
        {
            if (currentIndex == songIndex)
            {
                shouldReplace = true;
                break;
            }
        }

        if (shouldReplace)
        {
            _audio->play(songs::ERROR, devices::Hv20tPlayMode::StopThenPlay);
        }
        else
        {
            _audio->play(songs::ERROR, mode);
        }
    }

    void MarbleController::playLiftError(const String &errorCode)
    {
        if (errorCode == "LIFT_NO_ZERO")
        {
            playErrorSound(Hv20tPlayMode::QueueIfPlaying, {songs::LIFT_STOP});
            _audio->play(songs::LIFT_NO_ZERO, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else if (errorCode == "LIFT_INIT_NO_ZERO")
        {
            playErrorSound(Hv20tPlayMode::QueueIfPlaying, {songs::LIFT_STOP});
            _audio->play(songs::LIFT_INIT_ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
        }
    }

    void MarbleController::playWheelError(const String &errorCode)
    {
        if (errorCode == "CalibrationZeroNotFound")
        {
            _audio->play(songs::ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else if (errorCode == "CalibrationSecondZeroNotFound")
        {
            _audio->play(songs::ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else if (errorCode == "ZeroNotFound")
        {
            _audio->play(songs::ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_ZERO_NOT_FOUND, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else if (errorCode == "UnexpectedZeroTrigger")
        {
            _audio->play(songs::ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_UNEXPECTED_ZERO_TRIGGER, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        else
        {
            MLOG_ERROR("%s: Unknown Wheel errorCode '%s', cannot play audio", toString().c_str(), errorCode.c_str());
        }
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

    void MarbleController::playButtonDown(std::vector<int> additionalReplaceSongIndexes)
    {
        // Create the default replace list with button sounds
        std::vector<int> replaceSongIndexes = {songs::getButtonDownSound(), songs::getButtonUpSound(), songs::getButtonClickSound()};

        // Add any additional indexes
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());

        // Check if any song from the replace list is currently playing
        auto currentIndex = _audio->getPlayingIndex();
        bool shouldReplace = false;

        for (int songIndex : replaceSongIndexes)
        {
            if (currentIndex == songIndex)
            {
                shouldReplace = true;
                break;
            }
        }

        if (shouldReplace)
        {
            _audio->play(songs::getButtonDownSound(), devices::Hv20tPlayMode::StopThenPlay);
        }
        else
        {
            _audio->play(songs::getButtonDownSound(), devices::Hv20tPlayMode::SkipIfPlaying);
        }
    }

    void MarbleController::playButtonUp(std::vector<int> additionalReplaceSongIndexes)
    {
        // Create the default replace list with button sounds
        std::vector<int> replaceSongIndexes = {songs::getButtonDownSound(), songs::getButtonUpSound(), songs::getButtonClickSound()};

        // Add any additional indexes
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());

        // Check if any song from the replace list is currently playing
        auto currentIndex = _audio->getPlayingIndex();
        bool shouldReplace = false;

        for (int songIndex : replaceSongIndexes)
        {
            if (currentIndex == songIndex)
            {
                shouldReplace = true;
                break;
            }
        }

        if (shouldReplace)
        {
            _audio->play(songs::getButtonUpSound(), devices::Hv20tPlayMode::StopThenPlay);
        }
        else
        {
            _audio->play(songs::getButtonUpSound(), devices::Hv20tPlayMode::SkipIfPlaying);
        }
    }

    void MarbleController::playButtonClick(std::vector<int> additionalReplaceSongIndexes)
    {
        // Create the default replace list with button sounds
        std::vector<int> replaceSongIndexes = {songs::getButtonDownSound(), songs::getButtonUpSound(), songs::getButtonClickSound()};

        // Add any additional indexes
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());

        // Check if any song from the replace list is currently playing
        auto currentIndex = _audio->getPlayingIndex();
        bool shouldReplace = false;

        for (int songIndex : replaceSongIndexes)
        {
            if (currentIndex == songIndex)
            {
                shouldReplace = true;
                break;
            }
        }

        if (shouldReplace)
        {
            _audio->play(songs::getButtonClickSound(), devices::Hv20tPlayMode::StopThenPlay);
        }
        else
        {
            _audio->play(songs::getButtonClickSound(), devices::Hv20tPlayMode::SkipIfPlaying);
        }
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
        // If error is gone, remove queued error songs
        if (previousWheelState == devices::WheelStateEnum::ERROR &&
            wheelState->state != devices::WheelStateEnum::ERROR)
        {
            // Don't play error that are not active anymore
            _audio->removeFromQueue(songs::WHEEL_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_UNEXPECTED_ZERO_TRIGGER);
        }

        // * -> ERROR
        if (previousWheelState != devices::WheelStateEnum::ERROR &&
            wheelState->state == devices::WheelStateEnum::ERROR)
        {
            playWheelError(_wheel->getErrorCode());
        }

        // CALIBRATING -> IDLE
        if (previousWheelState == devices::WheelStateEnum::CALIBRATING &&
            wheelState->state == devices::WheelStateEnum::IDLE)
        {
            _audio->play(songs::NOTIFICATION, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_CALIBRATION_END, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        previousWheelState = wheelState->state;
    }

    void MarbleController::onLiftStateChange(void *statePtr)
    {
        static devices::LiftStateEnum previousLiftState = devices::LiftStateEnum::UNKNOWN;

        auto *liftState = static_cast<devices::LiftState *>(statePtr);
        if (!liftState)
        {
            return;
        }

        if (previousLiftState != devices::LiftStateEnum::LIFT_UP &&
            liftState->state == devices::LiftStateEnum::LIFT_UP &&
            liftState->isLoaded)
        {
            _audio->play(songs::LIFT_STOP, devices::Hv20tPlayMode::SkipIfPlaying);
        }

        if (previousLiftState != devices::LiftStateEnum::LIFT_DOWN &&
            liftState->state == devices::LiftStateEnum::LIFT_DOWN &&
            !liftState->isLoaded)
        {
            _audio->play(songs::LIFT_STOP, devices::Hv20tPlayMode::QueueIfPlaying);
        }

        // ERROR -> *
        // If error is gone, remove queued error songs
        // if (previousLiftState == devices::LiftStateEnum::ERROR &&
        //     liftState->state != devices::LiftStateEnum::ERROR)
        // {
        // }

        // * -> ERROR
        if (previousLiftState != devices::LiftStateEnum::ERROR &&
            liftState->state == devices::LiftStateEnum::ERROR)
        {
            playLiftError(_lift->getErrorCode());
        }

        previousLiftState = liftState->state;
    }

} // namespace devices
