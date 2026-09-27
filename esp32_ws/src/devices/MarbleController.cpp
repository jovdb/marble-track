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
#include "devices/Launcher.h"
#include "devices/WheelLoader.h"
#include "SongConstants.h"

extern DeviceManager deviceManager;

namespace devices
{

    namespace lift_timing
    {
        static constexpr unsigned long PowerSongDurationMs = 5600UL;
        static constexpr unsigned long PowerSongStartDelayMs = 500UL;
        static constexpr unsigned long AutoPowerSongStartDelayMs = 1000UL;
        static constexpr unsigned long AutoNoBallRandomMinDelayMs = 120000UL;
        static constexpr unsigned long AutoNoBallRandomMaxDelayMs = 300000UL;
        static constexpr unsigned long ErrorLongPressDurationMs = 5000UL;    // 5 seconds for error recovery
        static constexpr unsigned long LongPressAutoModeDurationMs = 3000UL; // 5 seconds for error recovery
        static constexpr float AutoDownNoBallSpeedRatio = 0.2f;
        static constexpr float AutoDownNormalSpeedRatio = 1.0f;
        static constexpr float LiftAutoSpeedRatio = 0.25f;
        static constexpr float LiftManualSpeedRatio = 1.0f;
    }

    namespace wheel_timing
    {
        static constexpr float AutoSpeedRatio = 0.6f;
    }

    MarbleController::MarbleController(const String &id) : Device(id, "marblecontroller")
    {
        _buzzer = new devices::Buzzer("buzzer");
        addChild(_buzzer);

        _audio = new devices::Hv20tAudio("hv20t");
        addChild(_audio);

        _powerMonitor = new devices::PowerMonitor("power");
        auto powerConfig = _powerMonitor->getConfig();
        powerConfig.name = "Power Monitor";
        powerConfig.i2cDeviceId = "i2c-1";
        powerConfig.i2cAddress = 68;
        powerConfig.shuntResistance = 0.1f;
        powerConfig.maxCurrent = 1;
        _powerMonitor->setConfig(powerConfig);
        addChild(_powerMonitor);

        _battery = new devices::Battery("battery");
        auto batteryConfig = _battery->getConfig();
        batteryConfig.name = "Battery";
        batteryConfig.powerMonitorDeviceId = "power";
        batteryConfig.minVoltage = 15.3f;
        batteryConfig.maxVoltage = 20.3f;
        batteryConfig.refreshIntervalMs = 20000;
        _battery->setConfig(batteryConfig);
        addChild(_battery);

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

        // Create launcher with default config
        _launcher = new devices::Launcher("launcher");

        for (Device *child : _launcher->getChildren())
        {
            if (!child)
                continue;

            if (child->getId() == "launcher-button" && child->getType() == "button")
            {
                auto *button = static_cast<devices::Button *>(child);
                auto buttonConfig = button->getConfig();
                buttonConfig.pinConfig.pin = 45;
                buttonConfig.pinConfig.expanderId = "";
                buttonConfig.name = "Launcher Ball Sensor";
                buttonConfig.debounceTimeInMs = 50;
                buttonConfig.pinMode = devices::PinModeOption::PullDown;
                buttonConfig.buttonType = devices::ButtonType::NormalOpen;
                button->setConfig(buttonConfig);
            }
            else if (child->getId() == "launcher-servo" && child->getType() == "servo")
            {
                auto *servo = static_cast<devices::Servo *>(child);
                auto servoConfig = servo->getConfig();
                servoConfig.pinConfig.pin = 46;
                servoConfig.pinConfig.expanderId = "";
                servoConfig.name = "Launcher Arm Servo";
                servoConfig.mcpwmChannel = 3;
                servoConfig.frequency = 50;
                servoConfig.resolutionBits = 10;
                servoConfig.minDutyCycle = 9.0f;
                servoConfig.maxDutyCycle = 4.7f;
                servoConfig.defaultDurationInMs = 500;
                servo->setConfig(servoConfig);
            }
        }

        auto launcherConfig = _launcher->getConfig();
        launcherConfig.name = "Launcher";
        launcherConfig.loadTimeMs = 2000;
        launcherConfig.launchTimeMs = 100;
        _launcher->setConfig(launcherConfig);

        addChild(_launcher);

        _wheelLoader = new devices::WheelLoader("wheel-loader");
        addChild(_wheelLoader);

        _launcherLed = new devices::Led("launcher-led");
        addChild(_launcherLed);

        _launcherBtn = new devices::Button("launcher-btn");
        addChild(_launcherBtn);
    }

    void MarbleController::setup()
    {
        Device::setup();

        if (_battery)
        {
            if (_battery->refresh())
            {
                if (_battery->getState().voltage > 0)
                {
                    if (_battery->getState().batteryPercent < ShutDownAtPercent)
                    {
                        MLOG_WARN("%s: Critical battery level (%.1f%% <= %.1f%%) detected during setup. Initiating shutdown sequence.", toString().c_str(), _battery->getState().batteryPercent, ShutDownAtPercent);
                        doPowerShutdown = true;
                        return;
                    }
                    else
                    {
                        MLOG_DEBUG("%s: Battery level OK (%.1f%% > %.1f%%)", toString().c_str(), _battery->getState().batteryPercent, ShutDownAtPercent);
                    }
                }
                else
                {
                    MLOG_WARN("%s: Battery voltage reading failed during setup", toString().c_str());
                }
            }
            else
            {
                MLOG_WARN("Battery refresh failed during setup");
            }
        }

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
        _autoLiftMovingDownSlow = false;

        // Set auto mode based on manual button state during setup
        isAutoMode = !_manualButton->isPressed();

        // Log the operating mode
        MLOG_INFO("%s Initialized in %s mode", toString().c_str(), isAutoMode ? "AUTO" : "MANUAL");

        // Initialize launcher: auto mode starts init after a delay, manual mode waits for button press
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
        _randomWheelDelayMs = 0;
        _lastButtonPressTime = 0;
        _idleSoundPlayed = false;
        isAutoMode = false;

        // Reset launcher timing

        // Reset splitter sensor variables
        _splitterCounter = 0;
    }

    void MarbleController::loop()
    {

        // Trigger Shutdown at low battery
        auto static shutdownMillis = 0;
        if (_battery)
        {
            if (!doPowerShutdown)
            {
                doPowerShutdown = shutdownMillis == 0 && _battery->getState().voltage > 0 && _battery->getState().batteryPercent < ShutDownAtPercent;
            }

            if (doPowerShutdown && shutdownMillis == 0)
            {
                MLOG_WARN("%s: Critical battery level (%.1f%% <= %.1f%%) detected during loop. Initiating shutdown sequence.", toString().c_str(), _battery->getState().batteryPercent, ShutDownAtPercent);

                // Play songs
                shutdownMillis = millis();
                _audio->play(songs::BATTERY_CRITICAL, devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::SHUTDOWN_TEXT, devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::SHUTDOWN, devices::Hv20tPlayMode::QueueIfPlaying);

                _liftLed->blink(50, 1300, 0);
                _wheelLed->blink(50, 1250, 50);
                _launcherLed->blink(50, 1200, 100);
                _spiralLed->blink(50, 1150, 150);
            }
            else if (shutdownMillis > 0 && millis() - shutdownMillis >= 10000UL)
            {
                // Shutdown
                esp_deep_sleep_start(); // stop until re-powered
            }
        }

        Device::loop();

        if (doPowerShutdown)
            return;

        // Check for idle timeout (5 minutes = 300000 ms)
        if (!isAutoMode && _lastButtonPressTime && (millis() - _lastButtonPressTime) > 300000UL && !_idleSoundPlayed)
        {
            _audio->play(songs::NOTIFICATION, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::IDLE, devices::Hv20tPlayMode::QueueIfPlaying);
            _idleSoundPlayed = true;
        }

        // Idle tracking: Any button pressed?
        if (_liftBtn->onPressed() || _wheelBtn->onPressed() || _launcherBtn->onPressed() || _spiralBtn->onPressed())
        {
            _lastButtonPressTime = millis();
            _idleSoundPlayed = false;
        }

        if (isAutoMode)
        {
            loopAutoLift();
            loopWheel(true);
            loopAutoSpiral();
            loopLauncher(true);
            loopWheelLoader(true);
        }
        else
        {
            loopManualLift();
            loopWheel(false);
            loopManualSpiral();
            loopLauncher(false);
            loopWheelLoader(false);
        }

        loopSplitter();
        loopBattery();
        loopConfigError();
    }

    void MarbleController::loopManualLift()
    {
        auto liftState = _lift->getState();

        // LED
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            _liftLed->set(true);
            break;
        }
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
        case devices::LiftStateEnum::MOVING_DOWN:
        case devices::LiftStateEnum::MOVING_UP:
            if (_autoLiftMovingDownSlow)
            {
                _liftLed->set(true);
            }
            else if (_isLiftTempAutoMode)
            {
                blinkBusy(_liftLed);
            }
            else
            {
                blinkLiftQueued();
            }

            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        {
            if (_isLiftTempAutoMode)
            {
                blinkBusy(_liftLed);
            }
            else if (_liftQueuedPresses > 0)
            {
                blinkLiftQueued();
            }
            else
            {
                _liftLed->set(true);
            }
            break;
        }
        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {
            static auto _liftBallWaitingNotificationFirst = 60000;
            static auto _liftBallWaitingNotificationRecurring = 120000;
            static auto _liftBallWaitingNotificationDuration = 3000;

            if (_liftBallReadyWaitingTime)
            {
                if (!_liftBtn->isPressed())
                {

                    if (
                        (_liftBallReadyWaitingTime + _liftBallWaitingNotificationFirst) < millis())
                    {
                        // New or again
                        if (!_playedLiftBallWaitingSoundAt || ((_playedLiftBallWaitingSoundAt + _liftBallWaitingNotificationRecurring) < millis()))
                        {
                            _audio->play(songs::LIFT_BALL_WAITING, devices::Hv20tPlayMode::QueueIfPlaying);
                            _playedLiftBallWaitingSoundAt = millis();
                        }
                    }
                }
            }

            // Attention
            if (_playedLiftBallWaitingSoundAt && _playedLiftBallWaitingSoundAt <= millis() && _playedLiftBallWaitingSoundAt + _liftBallWaitingNotificationDuration > millis())
            {
                blinkAttention(_liftLed);
            }
            // Lift Auto Mode
            else if (_isLiftTempAutoMode)
            {
                blinkBusy(_liftLed);
            }
            // Queued
            else if (_liftQueuedPresses > 0)
            {
                blinkLiftQueued();
            }
            else
            {
                // USer can press
                _liftLed->set(true);
            }

            break;
        }
        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        {
            static auto _liftBallWaitingNotificationFirst = 60000;
            static auto _liftBallWaitingNotificationRecurring = 120000;
            static auto _liftBallWaitingNotificationDuration = 960 * 5;

            if (_liftBallReadyWaitingTime)
            {
                if (liftState.ballWaitingSince > 0)
                {

                    auto mostRecent = std::max(liftState.ballWaitingSince, _liftBallReadyWaitingTime);
                    if (
                        (mostRecent + _liftBallWaitingNotificationFirst) < millis())
                    {
                        // New or again
                        if (!_playedLiftBallWaitingSoundAt || ((_playedLiftBallWaitingSoundAt + _liftBallWaitingNotificationRecurring) < millis()))
                        {
                            _audio->play(songs::LIFT_BALL_WAITING, devices::Hv20tPlayMode::QueueIfPlaying);
                            _playedLiftBallWaitingSoundAt = millis();
                        }
                    }
                }
            }

            // Attention
            if (_playedLiftBallWaitingSoundAt && _playedLiftBallWaitingSoundAt <= millis() && _playedLiftBallWaitingSoundAt + _liftBallWaitingNotificationDuration > millis())
            {
                blinkAttention(_liftLed);
            }
            // Lift Auto Mode
            else if (_isLiftTempAutoMode)
            {
                blinkBusy(_liftLed);
            }
            // Queued
            else if (_liftQueuedPresses > 0)
            {
                blinkLiftQueued();
            }
            else
            {
                // USer can press
                _liftLed->set(true);
            }

            break;
        }
        }
        // Reset button timing state when not in LIFT_UP
        if (liftState.state != devices::LiftStateEnum::LIFT_UP_EMPTY &&
            liftState.state != devices::LiftStateEnum::LIFT_UP_LOADED)
        {
            _isLiftPowerUnloadSongPlaying = false;
        }

        // Lift Logic
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            _liftQueuedPresses = 0;
            // Init will start at press
            if (_liftBtn->onPressed())
            {
                _lift->init(lift_timing::LiftManualSpeedRatio);
                playButtonClick();
            }
            break;
        }

        case devices::LiftStateEnum::ERROR:
        {
            _liftQueuedPresses = 0;

            // Pressed
            if (_liftBtn->onPressed())
            {
                _isLiftPressedDuringError = true;
            }

            // Short Press
            if (_isLiftPressedDuringError && _liftBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                playLiftError(_lift->getErrorCode());
            }

            // Check for long press while button is held
            if (_isLiftPressedDuringError && _liftBtn->onPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting lift init", toString().c_str());
                _lift->init(lift_timing::LiftAutoSpeedRatio);
                _audio->play(songs::LIFT_RESTART, devices::Hv20tPlayMode::StopThenPlay);
            }
            break;
        }
        case devices::LiftStateEnum::INIT:
            if (_liftBtn->onPressed())
            {
                playErrorSound(devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::LIFT_INIT_BUSY, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            break;

        // During actions
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        {
            if (_liftBtn->onPressed() && _liftQueuedPresses < 240)
            {
                // if only up, go back down, else whole cycle
                _liftQueuedPresses += _liftQueuedPresses == 1 ? 6 : 4;
                playButtonClick({songs::LIFT_STOP});
            }
            break;
        }
        case devices::LiftStateEnum::MOVING_UP:
        {
            if (_liftBtn->onPressed() && _liftQueuedPresses < 240)
            {
                // if only up, go back down, else whole cycle
                _liftQueuedPresses += _liftQueuedPresses == 0 ? 6 : 4;
                playButtonClick({songs::LIFT_STOP});
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            if (_liftBtn->onPressed() && _liftQueuedPresses < 240)
            {
                _liftQueuedPresses += 4; // whole cycle
                playButtonClick({songs::LIFT_STOP});
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        {
            bool isShortPress = false;

            // Down
            if (_liftBtn->onPressed())
            {
                if (_liftQueuedPresses)
                {
                    playButtonClick({songs::LIFT_STOP});
                }
                else
                {
                    playButtonDown({songs::LIFT_STOP});
                }
            }

            // Long Press
            if (_liftBtn->onPressedDuration(lift_timing::LongPressAutoModeDurationMs) && !_liftQueuedPresses)
            {
                if (_lift->isBallWaiting())
                {
                    // TODO, play sound
                    MLOG_INFO("%s: Starting lift auto mode", toString().c_str());
                    _audio->play(songs::LIFT_AUTO_MODE_START, devices::Hv20tPlayMode::StopThenPlay);
                    _isLiftTempAutoMode = true;

                    // calc max cycles to prevent overflow
                    auto cycles = (255 - _liftQueuedPresses) / 4;
                    _liftQueuedPresses += 4 * cycles; // max 60 cycles
                }
                else
                {
                    playErrorSound(devices::Hv20tPlayMode::StopThenPlay);
                }
            }

            // Short Press
            else if (_liftBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::LongPressAutoModeDurationMs) && _liftQueuedPresses < 240)
            {
                isShortPress = true;
                if (!_liftQueuedPresses)
                    playButtonUp({songs::LIFT_STOP});
                _liftQueuedPresses += _liftQueuedPresses == 0 ? 2 : 4; // to top
            }

            // Auto start next action
            if (_liftQueuedPresses > 0)
            {

                // Actions in queue or a manuel press
                if (_lift->isBallWaiting() || isShortPress)
                {
                    if (_lift->loadBall())
                    {
                        _liftQueuedPresses--;
                    }
                }
                else
                {
                    // If not Auto mode, wait for ball
                    // If in Auto mode, stop Automode
                    if (_isLiftTempAutoMode)
                    {
                        _liftQueuedPresses = 0;
                        _isLiftTempAutoMode = false;
                        if (_audio->getPlayingIndex() == songs::LIFT_STOP)
                        {
                            _audio->play(songs::LIFT_AUTO_MODE_END, devices::Hv20tPlayMode::StopThenPlay); // Play after bell
                        }
                        else
                        {
                            _audio->play(songs::LIFT_AUTO_MODE_END, devices::Hv20tPlayMode::QueueIfPlaying); // Play after bell
                        }
                    }
                }
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        {
            if (_liftBtn->onPressed() && _liftQueuedPresses < 240)
            {
                // if only up, go back down, else whole cycle
                _liftQueuedPresses += _liftQueuedPresses == 2 ? 6 : 4;
                playButtonClick({songs::LIFT_STOP});
            }

            // Auto start next action
            if (_liftQueuedPresses > 0)
            {
                if (_lift->up(lift_timing::LiftManualSpeedRatio))
                {
                    _liftQueuedPresses--;
                }
            }

            break;
        }

        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {
            // Down
            if (_liftBtn->onPressed())
            {
                if (_liftQueuedPresses)
                    playButtonClick({songs::LIFT_STOP});
                else
                    playButtonDown({songs::LIFT_STOP});
            }

            // Short Press
            if (_liftBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::PowerSongStartDelayMs) && _liftQueuedPresses < 240)
            {
                if (!_liftQueuedPresses)
                    playButtonUp({songs::LIFT_STOP});
                if (_liftQueuedPresses == 0)
                {
                    _liftQueuedPresses += 2; // top bottom
                }

                else
                {
                    _liftQueuedPresses += 4; // whole cycle
                }
            }

            if (_liftQueuedPresses > 0)
            {
                if (_lift->unloadBall(1.0f))
                {
                    _liftQueuedPresses--;
                }

                return;
            }

            // semi long Press, start sound
            if (_liftBtn->onPressedDuration(lift_timing::PowerSongStartDelayMs))
            {
                _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
            }

            // Cancelled long press
            else if (_liftBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::PowerSongDurationMs))
            {
                _audio->stop();
                playButtonUp({songs::LIFT_STOP, songs::LIFT_POWER_UNLOAD});
            }

            // Long Press
            if (_liftBtn->onPressedDuration(lift_timing::PowerSongDurationMs) && _liftQueuedPresses < 240)
            {
                MLOG_INFO("%s: Long press detected (%.2fs), Power unload", toString().c_str());
                // Long press: unload with full speed immediately
                _lift->unloadBall(0.2f);
                _liftQueuedPresses += 1; // unload +  bottom
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        {
            if (_liftBtn->onPressed() && _liftQueuedPresses < 240)
            {
                _liftQueuedPresses += 4; // whole cycle
                playButtonClick({songs::LIFT_STOP});
            }

            // Auto start next action
            if (_liftQueuedPresses > 0)
            {
                // If not loaded but still queued, try going down to load if possible
                if (_lift->down(lift_timing::LiftManualSpeedRatio))
                {
                    _liftQueuedPresses--;
                }
            }
            break;
        }
        }
    }

    void MarbleController::loopAutoLift()
    {
        // Auto lift control logic - automatic cycling through lift operations
        auto liftState = _lift->getState();

        if (liftState.state != devices::LiftStateEnum::LIFT_UP_LOADED)
        {
            _autoPowerUnloadPending = false;
            _autoPowerUnloadSongStarted = false;
            _autoPowerUnloadStartTime = 0;
            _autoLiftUpLoadedSince = 0;
        }

        if (liftState.state != devices::LiftStateEnum::LIFT_DOWN_EMPTY || liftState.ballWaitingSince > 0)
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
        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        case devices::LiftStateEnum::LIFT_UP_LOADED:
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

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
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

            // Pressed
            if (_liftBtn->onPressed())
            {
                _isLiftPressedDuringError = true;
            }

            // Short Press
            if (_isLiftPressedDuringError && _liftBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                playLiftError(_lift->getErrorCode());
            }

            // Check for long press while button is held
            if (_isLiftPressedDuringError && _liftBtn->onPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting lift init", toString().c_str());
                _lift->init(lift_timing::LiftAutoSpeedRatio);
                _audio->play(songs::LIFT_RESTART, devices::Hv20tPlayMode::StopThenPlay);
            }
            break;

        // BUSY states - just blink LED
        case devices::LiftStateEnum::INIT:
            if (_liftBtn->onPressed())
            {
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                _audio->play(songs::LIFT_INIT_BUSY, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
            if (_liftBtn->onPressed())
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
            else if (_liftBtn->onPressed())
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

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        {
            _isLiftPowerUnloadSongPlaying = false;

            if (liftState.state == devices::LiftStateEnum::LIFT_DOWN_LOADED)
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

                if (_liftBtn->onPressed())
                {
                    playButtonDown({songs::LIFT_STOP});
                }

                if (_liftBtn->onReleased())
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

        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {
            if (_liftBtn->onPressed())
                playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});

            // Check if we need to wait before next operation
            if (_autoLiftDelayStart > 0 && (millis() - _autoLiftDelayStart) < _autoLiftDelayMs)
            {
                // Still waiting, do nothing
                break;
            }

            if (liftState.state == devices::LiftStateEnum::LIFT_UP_LOADED)
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
    }

    void MarbleController::loopLauncher(bool autoMode)
    {

        // wheel inRange
        auto wheelState = _wheel->getState();

        const bool wheelInLaunchRange =
            (wheelState.state == devices::WheelStateEnum::IDLE || wheelState.state == devices::WheelStateEnum::MOVING) &&
            (LauncherWheelMaxAngle < 360
                 ? (wheelState.currentAngle >= LauncherWheelMinAngle &&
                    wheelState.currentAngle <= LauncherWheelMaxAngle)
                 : (wheelState.currentAngle >= LauncherWheelMinAngle ||
                    (wheelState.currentAngle <= LauncherWheelMaxAngle - 360)));

        const bool wheelInLoadRange =
            (wheelState.state == devices::WheelStateEnum::IDLE || wheelState.state == devices::WheelStateEnum::MOVING) &&
            (LauncherWheelLoadMaxAngle < 360
                 ? (wheelState.currentAngle >= LauncherWheelLoadMinAngle &&
                    wheelState.currentAngle <= LauncherWheelLoadMaxAngle)
                 : (wheelState.currentAngle >= LauncherWheelLoadMinAngle ||
                    (wheelState.currentAngle <= LauncherWheelLoadMaxAngle - 360)));

        MLOG_DEBUG("wheelInLaunchRange: %d, wheelState.currentAngle: %f", wheelInLaunchRange, wheelState.currentAngle);
        auto launcherState = _launcher->getState();
        auto static launcherLastDownMillis = 0;

        static bool isBallLaunched = false;
        // Reset
        if (!wheelInLaunchRange)
        {
            isBallLaunched = false;
        }

        // LED
        switch (launcherState.state)
        {
        case LauncherStateEnum::UNKNOWN:
            _launcherLed->set(false);
            break;
        case LauncherStateEnum::ERROR:
            blinkError(_launcherLed);
            break;
        case LauncherStateEnum::MOVING_UP:
        case LauncherStateEnum::UP:
        case LauncherStateEnum::MOVING_DOWN:
            blinkBusy(_launcherLed);
            break;
        case LauncherStateEnum::DOWN:
            if (wheelState.state != devices::WheelStateEnum::MOVING && wheelState.state != devices::WheelStateEnum::IDLE)
            {
                // No led during wheel initializing, error, ...
                _launcherLed->set(false);
            }
            else if (wheelInLaunchRange && launcherState.isBallLoaded && !isBallLaunched)
            {
                // Can Launch
                _launcherLed->set(true);
            }
            else
            {
                // Can't do anything
                _launcherLed->set(false);
            }
            break;
        }

        // Button logic
        switch (launcherState.state)
        {
        case LauncherStateEnum::UNKNOWN:
            // Auto init at start (delay to not all start at the same time)
            if (millis() > 7000)
                _launcher->init();
            break;
        case LauncherStateEnum::ERROR:
        case LauncherStateEnum::MOVING_UP:
        case LauncherStateEnum::UP:
        case LauncherStateEnum::MOVING_DOWN:
            if (_launcherBtn->onPressed())
            {
                MLOG_INFO("%s: Cannot perform action, launcher is busy or in error state", toString().c_str());
                playErrorSound();
            }
            break;
        case LauncherStateEnum::DOWN:

            if (launcherLastDownMillis == 0)
            {
                launcherLastDownMillis = millis();
            }

            // Auto load ball if ball is waiting and not loaded yet ( and we know wheel position)
            if ((wheelState.state == devices::WheelStateEnum::MOVING || wheelState.state == devices::WheelStateEnum::IDLE) &&
                wheelInLoadRange &&
                !launcherState.isBallLoaded &&
                launcherState.isBallWaiting)
            {
                _launcher->load();
            }
            else
            {
                if (!autoMode)
                {
                    if (wheelInLaunchRange)
                    {
                        if (_launcherBtn->onPressed())
                        {
                            if (launcherState.isBallLoaded)
                            {
                                if (isBallLaunched)
                                {
                                    playErrorSound();
                                    MLOG_INFO("%s: Ball already launched", toString().c_str());
                                }
                                else
                                {
                                    _audio->play(songs::LAUNCH, devices::Hv20tPlayMode::SkipIfPlaying);
                                    if (_launcher->launch())
                                    {
                                        isBallLaunched = true;
                                    }
                                }
                            }
                            else
                            {
                                MLOG_INFO("%s: Cannot launch, no ball", toString().c_str());
                                playErrorSound();
                            }
                        }
                    }
                    else
                    {
                        if (_launcherBtn->onPressed())
                        {
                            MLOG_INFO("%s: Cannot launch, landing platform is not in range", toString().c_str());
                            playErrorSound();
                        }
                    }

                    break;
                }
                else
                {
                    // Auto launch
                    if (launcherState.isBallLoaded && !isBallLaunched)
                    {
                        // -1: Out range
                        // 0: Start of range
                        // 1: End of range
                        const auto rangeRatio =
                            wheelInLaunchRange ? (wheelState.currentAngle - LauncherWheelMinAngle) /
                                                     (LauncherWheelMaxAngle - LauncherWheelMinAngle)
                                               : -1;

                        auto delay = !isBallLaunched ? 0 : 1000;
                        if (rangeRatio >= 0.2 && rangeRatio <= 0.8 && millis() - launcherLastDownMillis >= delay)
                        {
                            _audio->play(songs::LAUNCH, devices::Hv20tPlayMode::SkipIfPlaying);
                            if (_launcher->launch())
                            {
                                isBallLaunched = true;
                            }
                        }
                    }

                    if (_launcherBtn->onPressed())
                    {
                        MLOG_INFO("%s: Cannot launch in auto mode", toString().c_str());
                        playErrorSound();
                    }
                }
            }
        }

        if (launcherState.state != LauncherStateEnum::DOWN)
        {
            launcherLastDownMillis = 0;
        }
    }

    void MarbleController::loopWheelLoader(bool /*autoMode*/)
    {
        auto loaderState = _wheelLoader->getState();
        auto wheelState = _wheel->getState();

        // Auto init at start
        if (loaderState.state == WheelLoaderStateEnum::UNKNOWN)
        {
            _wheelLoader->init();
            return;
        }

        if (loaderState.state != WheelLoaderStateEnum::IDLE)
        {
            return;
        }

        // Not during wheel init / calibration
        if (wheelState.state == devices::WheelStateEnum::MOVING || wheelState.state == devices::WheelStateEnum::IDLE)
        {
            // Check if wheel angle is in range
            const bool isInRange1 = wheelState.currentAngle >= WheelLoaderRange1Min && wheelState.currentAngle <= WheelLoaderRange1Max;
            const bool isInRange2 = wheelState.currentAngle >= WheelLoaderRange2Min && wheelState.currentAngle <= WheelLoaderRange2Max;

            static bool wasInRange1 = false;
            static bool wasInRange2 = false;

            if (isInRange1 && !wasInRange1)
            {
                MLOG_INFO("%s: Wheel at angle %.2f, loading Wheel", toString().c_str(), wheelState.currentAngle);
                _wheelLoader->loadAny();
            }
            else if (isInRange2 && !wasInRange2)
            {
                MLOG_INFO("%s: Wheel at angle %.2f, loading wheel", toString().c_str(), wheelState.currentAngle);
                _wheelLoader->loadAny();
            }

            wasInRange1 = isInRange1;
            wasInRange2 = isInRange2;
        }
    }

    void MarbleController::loopWheel(bool autoMode = false)
    {
        auto wheelState = _wheel->getState();
        // 0 = not idle, >0 = idle start time
        unsigned long static wheelIdleStartTime = 0;

        // LED
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            _wheelLed->set(true); // clickable: init will start
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

            if (!autoMode && wheelIdleStartTime > 0 && (millis() - wheelIdleStartTime) >= 60000)
            {
                blinkAttention(_wheelLed);
            }
            else
            {
                _wheelLed->set(true);
            }
            break;

        default:
            MLOG_ERROR("%s: Unknown wheel state: %d", toString().c_str(), static_cast<int>(wheelState.state));
            _wheelLed->set(false); // LED off for any other state
        }

        /** Wheel speed, reduced in auto mode */
        auto modeSpeed = autoMode ? wheel_timing::AutoSpeedRatio : 1.0f;

        // Wheel button
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            if (autoMode)
            {
                // Auto init after 1 second
                // to prevent physical collision with loader initializing
                if (millis() > 3000)
                {
                    _wheel->init(-1, modeSpeed);
                }
            }
            else
            {
                if (_wheelBtn->onPressed())
                {
                    _wheel->init(-1, modeSpeed);
                }
            }
            break;

        case devices::WheelStateEnum::IDLE:

            // Remember when idle start:
            // - Auto mode: trigger next breakpoint at random delay
            // - Manual Mode:  blink after some time idle
            if (wheelIdleStartTime == 0)
            {
                // First
                wheelIdleStartTime = millis();
                if (autoMode)
                {
                    _randomWheelDelayMs = 3000 + random(100, 30000);
                    MLOG_INFO("%s: Next random wheel trigger starts in %.ds", toString().c_str(), _randomWheelDelayMs / 1000);
                }
            }

            if (!autoMode)
            {
                if (_wheelBtn->onPressed())
                {
                    // Button just pressed - start continuous movement until button is released
                    MLOG_INFO("%s: Starting manual wheel movement as long button is pressed", toString().c_str());

                    playButtonDown();
                    _wheel->move(100000, modeSpeed); // Large positive number for continuous movement
                }
            }
            else
            {
                // When idle, wait for random delay then trigger next breakpoint
                if (_randomWheelDelayMs > 0 && millis() >= wheelIdleStartTime + _randomWheelDelayMs)
                {
                    MLOG_INFO("%s: Goto wheel next breakpoint", toString().c_str());
                    _wheel->nextBreakPoint(modeSpeed);
                    _randomWheelDelayMs = 0;
                }
                else if (_wheelBtn->onPressed())
                {
                    // Idle + auto mode, also allow to start
                    playButtonClick({songs::LIFT_STOP});
                    _wheel->nextBreakPoint(modeSpeed);
                }
            }
            break;

        case devices::WheelStateEnum::MOVING:
            if (!autoMode)
            {
                if (_wheelBtn->onReleased())
                {
                    playButtonUp();

                    // Longpress?
                    if (_wheelBtn->onPressedDuration(WHEEL_SPIN_LONG_PRESS_MS))
                    {
                        // Button released - stop the wheel if it was a short press
                        MLOG_INFO("%s: Press released - stopping wheel", toString().c_str());
                        _wheel->stop();
                    }
                    // Short press
                    else
                    {
                        // next breakpoint on long press release
                        MLOG_INFO("%s: short press released - moving to next breakpoint", toString().c_str());
                        _wheel->nextBreakPoint(modeSpeed);
                    }
                }
                else if (_wheelBtn->onPressed())
                {
                    // Pressed during deceleration
                    playButtonDown();
                    _wheel->move(100000, modeSpeed);
                }
            }
            else
            {
                if (_wheelBtn->onPressed())
                {
                    // during movement no press allowed in auto mode
                    playErrorSound();
                }
            }
            break;
        case devices::WheelStateEnum::ERROR:
            // Pressed
            if (_wheelBtn->onPressed())
            {
                _isWheelPressedDuringError = true;
            }

            // Short Press
            if (_isWheelPressedDuringError && _wheelBtn->onReleased() && !_liftBtn->isLastPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                playWheelError(_wheel->getErrorCode());
            }

            // Check for long press while button is held
            if (_isWheelPressedDuringError && _wheelBtn->onPressedDuration(lift_timing::ErrorLongPressDurationMs))
            {
                MLOG_INFO("%s: Error recovery long press detected, starting wheel init", toString().c_str());
                _wheel->init(-1, modeSpeed);
                _audio->play(songs::WHEEL_RESTART, devices::Hv20tPlayMode::StopThenPlay);
            }
            break;
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
            if (_wheelBtn->onPressed())
            {
                playErrorSound();
            }
            break;
        default:
            break;
        }

        // Reset Wheel idle time
        if (wheelState.state != devices::WheelStateEnum::IDLE)
        {
            wheelIdleStartTime = 0;
        }
    }

    void MarbleController::loopAutoSpiral()
    {
    }

    void MarbleController::loopManualSpiral()
    {
        if (_spiralBtn->onPressed())
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

        static auto splitterErrorRetryCount = 0;
        static auto lastCountTime = 0;
        static auto nextSplitterRunTime = 0;

        // onPressed: queue
        if (_splitterSensor->onPressed())
        {
            _splitterCounter++;
            lastCountTime = millis();
            nextSplitterRunTime = lastCountTime + 500;
        }
        if (_splitterSensor->onReleased())
        {
            lastCountTime = 0;
        }

        switch (_splitter->getState().state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            if (millis() > 4000)
            {
                _splitter->init();
            }
            break;
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::CALIBRATING:
            break;
        case devices::WheelStateEnum::ERROR:
            MLOG_ERROR("%s: Splitter in ERROR state, reinitializing", toString().c_str());
            if (splitterErrorRetryCount < 3)
            {
                _splitter->init(); // TODO: Add delay?
                splitterErrorRetryCount++;
            }
            else
            {
                MLOG_ERROR("%s: Splitter failed to reinitialize after 3 attempts", toString().c_str());
                playErrorSound();
                _audio->play(songs::SPLITTER_ERROR);
            }
            break;

        case devices::WheelStateEnum::IDLE:
            splitterErrorRetryCount = 0;

            // Process queue
            if (_splitterCounter > 0)
            {
                if (!nextSplitterRunTime || (nextSplitterRunTime < millis()))
                {
                    _splitterCounter--;
                    _splitter->nextBreakPoint();
                    nextSplitterRunTime = 0;
                }
            }
            else
            {

                // Queue empty and still pressed: interval every 10s
                if (_splitterSensor->isPressed())
                {
                    if (lastCountTime + 10000 < millis())
                    {
                        _splitterCounter++;
                        lastCountTime = millis();
                    }
                }
            }
            break;

        case devices::WheelStateEnum::MOVING:
            // wait until idle for the next step
            break;
        }
    }

    void MarbleController::loopBattery()
    {
        if (_battery == nullptr)
            return;

        auto batteryState = _battery->getState();

        // At startup it is 0 by default, wait until ready and dat is available
        if (batteryState.status != "Ready" || batteryState.voltage == 0)
            return;

        long static nextCriticalMillis = 0;
        long static nextLowMillis = 0;
        long static nextStatusLogMillis = 0;

        auto now = millis();

        if (nextStatusLogMillis < now)
        {
            MLOG_INFO("%s: Battery level: %.2f%%", toString().c_str(), batteryState.batteryPercent);
            nextStatusLogMillis = now + 300000; // Log every 5 minutes
        }

        if (batteryState.batteryPercent < 10.0f)
        {
            if (nextCriticalMillis < now)
            {
                nextCriticalMillis = now + 180000; // Play every 3 minutes
                MLOG_WARN("%s: Battery critical (%.2f%%)", toString().c_str(), batteryState.batteryPercent);
                _audio->play(songs::BATTERY_CRITICAL, devices::Hv20tPlayMode::QueueIfPlaying);
            }
        }
        else if (batteryState.batteryPercent < 20.0f)
        {
            if (nextLowMillis < now)
            {
                nextLowMillis = now + 600000; // Play every 10 minutes
                MLOG_WARN("%s: Battery low (%.2f%%)", toString().c_str(), batteryState.batteryPercent);
                _audio->play(songs::BATTERY_LOW, devices::Hv20tPlayMode::QueueIfPlaying);
            }
        }
    }

    void MarbleController::loopConfigError()
    {
        bool static didPlayConfigError = false;

        // Loop over all devices and check if there is a config error
        // Play error once after boot
        if (!didPlayConfigError)
        {
            for (auto &device : getChildren())
            {
                auto errorCode = device->getErrorCode();
                if (errorCode == "CONFIG_ERROR")
                {
                    MLOG_ERROR("%s: Device '%s' has a config error, play sound: %s", toString().c_str(), device->getName().c_str(), device->getErrorMessage().c_str());
                    _audio->play(songs::CONFIG_ERROR, devices::Hv20tPlayMode::QueueIfPlaying);
                    didPlayConfigError = true;
                    break;
                }
            }
        }
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

        ledDevice->blink(720, 240);
    }

    void MarbleController::blinkAttention(Led *ledDevice)
    {
        if (!ledDevice)
        {
            return;
        }

        ledDevice->blink(360, 120); // Needs attention
    }

    void MarbleController::blinkLiftQueued()
    {
        if (!_liftLed)
            return;

        int offset = 0;
        switch (_lift->getState().state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        case devices::LiftStateEnum::ERROR:
        case devices::LiftStateEnum::INIT:
            break;
        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
            offset = -4;
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
            offset = -3;
            break;
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::LIFT_UP_LOADED:
            offset = -2;
            break;
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::LIFT_UP_EMPTY:
            offset = -1;
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            break;
        }

        // +3 to have 1 is queue > 0
        int queued = 1;
        if (_liftQueuedPresses > 0)
        {
            queued = (_liftQueuedPresses + offset) / 4 + 1; // floor
        }

        // if (queued > 10)
        //     queued = 10;

        std::vector<int> pattern;
        for (auto i = 0; i < queued; i++)
        {
            pattern.push_back(240);                         // On
            pattern.push_back(i == queued - 1 ? 880 : 240); // Off
        }

        _liftLed->pattern(pattern);
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
            playErrorSound();
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

            _isWheelPressedDuringError = false;
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

        if (previousLiftState != devices::LiftStateEnum::LIFT_UP_LOADED &&
            liftState->state == devices::LiftStateEnum::LIFT_UP_LOADED)
        {
            _liftBallReadyWaitingTime = millis();
            _audio->play(songs::LIFT_STOP, devices::Hv20tPlayMode::SkipIfPlaying);
        }

        if (previousLiftState != devices::LiftStateEnum::LIFT_DOWN_EMPTY &&
            liftState->state == devices::LiftStateEnum::LIFT_DOWN_EMPTY)
        {
            _liftBallReadyWaitingTime = millis();
            _audio->play(songs::LIFT_STOP, devices::Hv20tPlayMode::QueueIfPlaying);
        }

        if (previousLiftState == devices::LiftStateEnum::LIFT_DOWN_EMPTY &&
            liftState->state != devices::LiftStateEnum::LIFT_DOWN_EMPTY)
        {
            _playedLiftBallWaitingSoundAt = 0;
        }

        // ERROR -> *
        if (previousLiftState == devices::LiftStateEnum::ERROR &&
            liftState->state != devices::LiftStateEnum::ERROR)
        {
            _isLiftPressedDuringError = false;
            _audio->removeFromQueue(songs::LIFT_NO_ZERO);
        }

        // * -> ERROR
        if (previousLiftState != devices::LiftStateEnum::ERROR &&
            liftState->state == devices::LiftStateEnum::ERROR)
        {
            playLiftError(_lift->getErrorCode());
        }

        previousLiftState = liftState->state;
    }

} // namespace devices
