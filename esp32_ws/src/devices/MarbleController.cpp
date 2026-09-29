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
    MarbleController::MarbleController(const String &id) : Device(id, "marblecontroller")
    {
        _buzzer = new devices::Buzzer("buzzer");
        addChild(_buzzer);

        _audio = new devices::Hv20tAudio("hv20t");
        addChild(_audio);

        _trackAudio = std::make_unique<TrackAudio>(*_audio, *_buzzer);

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

        auto *lift = new devices::Lift("lift");
        addChild(lift);

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

        _trackLeds = std::make_unique<TrackLeds>(*_liftLed, *_wheelLed, *_launcherLed, *_spiralLed);
        _trackLift = std::make_unique<TrackLift>(
            *lift, *_liftBtn, *_liftLed, *_audio, *_trackAudio, *_trackLeds);
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
        now = millis();
        _lastButtonPressTime = now;
        _idleSoundPlayed = false;

        _trackLift->setup();

        // Initialize splitter sensor variables
        _trackSplitterState.queueCount = 0;

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
        _trackLift->teardown();
        Device::teardown();
        _trackWheelState.randomDelayMs = 0;
        _lastButtonPressTime = 0;
        _idleSoundPlayed = false;
        isAutoMode = false;

        // Reset launcher timing

        // Reset splitter sensor variables
        _trackSplitterState.queueCount = 0;
    }

    void MarbleController::loop()
    {
        now = millis();

        // Trigger Shutdown at low battery
        if (_battery)
        {
            if (!doPowerShutdown)
            {
                doPowerShutdown = _trackBatteryState.shutdownStartTimeMs == 0 && _battery->getState().voltage > 0 && _battery->getState().batteryPercent < ShutDownAtPercent;
            }

            if (doPowerShutdown && _trackBatteryState.shutdownStartTimeMs == 0)
            {
                MLOG_WARN("%s: Critical battery level (%.1f%% <= %.1f%%) detected during loop. Initiating shutdown sequence.", toString().c_str(), _battery->getState().batteryPercent, ShutDownAtPercent);

                // Play songs
                _trackBatteryState.shutdownStartTimeMs = now;
                _audio->play(songs::BATTERY_CRITICAL, devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::SHUTDOWN_TEXT, devices::Hv20tPlayMode::QueueIfPlaying);
                _audio->play(songs::SHUTDOWN, devices::Hv20tPlayMode::QueueIfPlaying);

                _trackLeds->blinkLoopAll();
            }
            else if (_trackBatteryState.shutdownStartTimeMs > 0 && now - _trackBatteryState.shutdownStartTimeMs >= 10000UL)
            {
                // Shutdown
                esp_deep_sleep_start(); // stop until re-powered
            }
        }

        Device::loop();

        if (doPowerShutdown)
            return;

        // Check for idle timeout (5 minutes = 300000 ms)
        if (!isAutoMode && _lastButtonPressTime && (now - _lastButtonPressTime) > 300000UL && !_idleSoundPlayed)
        {
            _audio->play(songs::NOTIFICATION, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::IDLE, devices::Hv20tPlayMode::QueueIfPlaying);
            _idleSoundPlayed = true;
        }

        // Idle tracking: Any button pressed?
        if (_liftBtn->onPressed() || _wheelBtn->onPressed() || _launcherBtn->onPressed() || _spiralBtn->onPressed())
        {
            _lastButtonPressTime = now;
            _idleSoundPlayed = false;
        }

        if (isAutoMode)
        {
            _trackLift->loopAutoMode(now);
            loopWheel(true);
            loopAutoSpiral();
            loopLauncher(true);
            loopWheelLoader(true);
        }
        else
        {
            _trackLift->loopManualMode(now);
            loopWheel(false);
            loopManualSpiral();
            loopLauncher(false);
            loopWheelLoader(false);
        }

        loopSplitter();
        loopBattery();
        loopConfigError();
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

        auto launcherState = _launcher->getState();
        // Reset
        if (!wheelInLaunchRange)
        {
            _trackLauncherState.isBallLaunched = false;
        }

        // LED
        switch (launcherState.state)
        {
        case LauncherStateEnum::UNKNOWN:
            _launcherLed->set(false);
            break;
        case LauncherStateEnum::ERROR:
            _trackLeds->blinkError(_launcherLed);
            break;
        case LauncherStateEnum::MOVING_UP:
        case LauncherStateEnum::UP:
        case LauncherStateEnum::MOVING_DOWN:
            if (_trackLauncherState.queueCount)
            {
                _trackLeds->blinkCount(_launcherLed, _trackLauncherState.queueCount);
            }
            else
            {
                _trackLeds->blinkBusy(_launcherLed);
            }
            break;
        case LauncherStateEnum::DOWN:
            if (_trackLauncherState.queueCount)
            {
                _trackLeds->blinkCount(_launcherLed, _trackLauncherState.queueCount);
            }
            else
            {
                if (wheelState.state != devices::WheelStateEnum::MOVING && wheelState.state != devices::WheelStateEnum::IDLE)
                {
                    // No led during wheel initializing, error, ...
                    _launcherLed->set(false);
                }
                else if (wheelInLaunchRange && launcherState.isBallLoaded && !_trackLauncherState.isBallLaunched)
                {
                    // Can Launch
                    _launcherLed->set(true);
                }
                else
                {
                    // Can't do anything
                    _launcherLed->set(false);
                }
            }
            break;
        }

        // Button logic
        switch (launcherState.state)
        {
        case LauncherStateEnum::UNKNOWN:
            // Auto init at start (delay to not all start at the same time)
            if (now > 7000)
                _launcher->init();
            break;
        case LauncherStateEnum::ERROR:
            if (_launcherBtn->onPressed())
            {
                MLOG_INFO("%s: Cannot perform action, launcher is busy or in error state", toString().c_str());
                playErrorSound();
            }
            break;
        case LauncherStateEnum::MOVING_UP:
        case LauncherStateEnum::UP:
        case LauncherStateEnum::MOVING_DOWN:
            if (_launcherBtn->onPressed())
            {
                _trackLauncherState.queueCount++;
                MLOG_INFO("%s: Increased launch queue to: %ul", toString().c_str(), _trackLauncherState.queueCount);
                playButtonCountClick(_trackLauncherState.queueCount);
            }
            break;
        case LauncherStateEnum::DOWN:

            if (_trackLauncherState.lastDownTimeMs == 0)
            {
                _trackLauncherState.lastDownTimeMs = now;
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
                    if (_launcherBtn->onPressed())
                    {
                        if (wheelInLaunchRange)
                        {
                            if (launcherState.isBallLoaded)
                            {
                                if (_trackLauncherState.isBallLaunched)
                                {
                                    MLOG_INFO("%s: Already launched, don't queue", toString().c_str(), _trackLauncherState.queueCount);
                                    playErrorSound();
                                }
                                else
                                {
                                    MLOG_INFO("%s: Launch triggered", toString().c_str());
                                    _trackLauncherState.queueCount++;
                                }
                            }
                            else
                            {
                                _trackLauncherState.queueCount++;
                                MLOG_INFO("%s: No ball, launch queued: %ul", toString().c_str(), _trackLauncherState.queueCount);
                                playButtonCountClick(_trackLauncherState.queueCount);
                            }
                        }
                        else
                        {
                            _trackLauncherState.queueCount++;
                            MLOG_INFO("%s: Increased launch queue to: %ul", toString().c_str(), _trackLauncherState.queueCount);
                            playButtonCountClick(_trackLauncherState.queueCount);
                        }
                    }
                    else
                    {
                        // Auto launch first possible launch
                        if (!_trackLauncherState.didInitLaunch)
                        {
                            // Wait until arm down
                            if (wheelInLaunchRange && !_trackLauncherState.isBallLaunched && launcherState.isBallLoaded && launcherState.state == devices::LauncherStateEnum::DOWN)
                            {
                                // Is in middle of range?
                                // Use 2.0f and 360.0f to ensure floating-point division and types match
                                auto launchAngle = std::fmod((LauncherWheelMaxAngle + LauncherWheelMinAngle) / 2.0f, 360.0f);
                                if (wheelState.currentAngle >= launchAngle)
                                {
                                    MLOG_INFO("%s: First launch triggered", toString().c_str());
                                    if (_launcher->launch())
                                    {
                                        _audio->play(songs::LAUNCH, devices::Hv20tPlayMode::SkipIfPlaying);
                                        _trackLauncherState.isBallLaunched = true;
                                        if (_trackLauncherState.queueCount > 0)
                                        {
                                            _trackLauncherState.didInitLaunch = true;
                                            _trackLauncherState.queueCount--;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Process queue
                            if (_trackLauncherState.queueCount &&
                                wheelInLaunchRange &&
                                launcherState.isBallLoaded &&
                                !_trackLauncherState.isBallLaunched &&
                                launcherState.isLoadingStep == 0) // Loaded complete delay ended
                            {
                                if (_launcher->launch())
                                {
                                    _audio->play(songs::LAUNCH, devices::Hv20tPlayMode::SkipIfPlaying);
                                    _trackLauncherState.isBallLaunched = true;
                                    _trackLauncherState.queueCount--;
                                }
                            }
                        }
                    }
                }
                else
                {

                    // Auto mode: launch
                    if (launcherState.isBallLoaded && !_trackLauncherState.isBallLaunched)
                    {
                        // -1: Out range
                        // 0: Start of range
                        // 1: End of range
                        const auto rangeRatio =
                            wheelInLaunchRange ? (wheelState.currentAngle - LauncherWheelMinAngle) /
                                                     (LauncherWheelMaxAngle - LauncherWheelMinAngle)
                                               : -1;

                        auto delay = !_trackLauncherState.isBallLaunched ? 0 : 1000;
                        if (rangeRatio >= 0.2 && rangeRatio <= 0.8 && now - _trackLauncherState.lastDownTimeMs >= delay)
                        {
                            _audio->play(songs::LAUNCH, devices::Hv20tPlayMode::SkipIfPlaying);
                            if (_launcher->launch())
                            {
                                _trackLauncherState.isBallLaunched = true;
                            }
                        }
                    }

                    if (_launcherBtn->onPressed())
                    {
                        MLOG_INFO("%s: Cannot launch in auto mode", toString().c_str());
                        playErrorSound();
                    }
                }
                break;
            }
        }

        if (launcherState.state != LauncherStateEnum::DOWN)
        {
            _trackLauncherState.lastDownTimeMs = 0;
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

            if (isInRange1 && !_trackWheelLoaderState.prevIsInRange1)
            {
                MLOG_INFO("%s: Wheel at angle %.2f, loading Wheel", toString().c_str(), wheelState.currentAngle);
                _wheelLoader->loadAny();
            }
            else if (isInRange2 && !_trackWheelLoaderState.prevIsInRange2)
            {
                MLOG_INFO("%s: Wheel at angle %.2f, loading wheel", toString().c_str(), wheelState.currentAngle);
                _wheelLoader->loadAny();
            }

            _trackWheelLoaderState.prevIsInRange1 = isInRange1;
            _trackWheelLoaderState.prevIsInRange2 = isInRange2;
        }
    }

    void MarbleController::loopWheel(bool autoMode = false)
    {
        auto wheelState = _wheel->getState();
        // 0 = not idle, >0 = idle start time
        // LED
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            _wheelLed->set(true); // clickable: init will start
            break;
        case devices::WheelStateEnum::ERROR:
            _trackLeds->blinkError(_wheelLed);
            break;
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
            _trackLeds->blinkInit(_wheelLed);
            break;
        case devices::WheelStateEnum::MOVING:
            _trackLeds->blinkBusy(_wheelLed);
            break;
        case devices::WheelStateEnum::IDLE:

            if (!autoMode && _trackWheelState.idleStartTimeMs > 0 && (now - _trackWheelState.idleStartTimeMs) >= 60000)
            {
                _trackLeds->blinkAttention(_wheelLed);
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
        auto modeSpeed = autoMode ? TrackWheelState::AUTO_SPEED_RATIO : 1.0f;

        // Wheel button
        switch (wheelState.state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            if (autoMode)
            {
                // Auto init after 1 second
                // to prevent physical collision with loader initializing
                if (now > 3000)
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
            if (_trackWheelState.idleStartTimeMs == 0)
            {
                // First
                _trackWheelState.idleStartTimeMs = now;
                if (autoMode)
                {
                    _trackWheelState.randomDelayMs = 3000 + random(100, 30000);
                    MLOG_INFO("%s: Next random wheel trigger starts in %.ds", toString().c_str(), _trackWheelState.randomDelayMs / 1000);
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
                if (_trackWheelState.randomDelayMs > 0 && now >= _trackWheelState.idleStartTimeMs + _trackWheelState.randomDelayMs)
                {
                    MLOG_INFO("%s: Goto wheel next breakpoint", toString().c_str());
                    _wheel->nextBreakPoint(modeSpeed);
                    _trackWheelState.randomDelayMs = 0;
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
                    if (_wheelBtn->onPressedDuration(TrackWheelState::WHEEL_SPIN_LONG_PRESS_MS))
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
                _trackWheelState.isPressedDuringError = true;
            }

            // Short Press
            if (_trackWheelState.isPressedDuringError && _wheelBtn->onShortClick(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
            {
                playWheelError(_wheel->getErrorCode());
            }

            // Check for long press while button is held
            if (_trackWheelState.isPressedDuringError && _wheelBtn->onPressedDuration(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
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
            _trackWheelState.idleStartTimeMs = 0;
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

        // onPressed: queue
        if (_splitterSensor->onPressed())
        {
            _trackSplitterState.queueCount++;
            _trackSplitterState.lastCountTimeMs = now;
            _trackSplitterState.nextRunTimeMs = _trackSplitterState.lastCountTimeMs + 500;
        }
        if (_splitterSensor->onReleased())
        {
            _trackSplitterState.lastCountTimeMs = 0;
        }

        switch (_splitter->getState().state)
        {
        case devices::WheelStateEnum::UNKNOWN:
            if (now > 4000)
            {
                _splitter->init();
            }
            break;
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::CALIBRATING:
            break;
        case devices::WheelStateEnum::ERROR:
            MLOG_ERROR("%s: Splitter in ERROR state, reinitializing", toString().c_str());
            if (_trackSplitterState.errorRetryCount < 3)
            {
                _splitter->init(); // TODO: Add delay?
                _trackSplitterState.errorRetryCount++;
            }
            else
            {
                MLOG_ERROR("%s: Splitter failed to reinitialize after 3 attempts", toString().c_str());
                playErrorSound();
                _audio->play(songs::SPLITTER_ERROR);
            }
            break;

        case devices::WheelStateEnum::IDLE:
            _trackSplitterState.errorRetryCount = 0;

            // Process queue
            if (_trackSplitterState.queueCount > 0)
            {
                if (!_trackSplitterState.nextRunTimeMs || (_trackSplitterState.nextRunTimeMs < now))
                {
                    _trackSplitterState.queueCount--;
                    _splitter->nextBreakPoint();
                    _trackSplitterState.nextRunTimeMs = 0;
                }
            }
            else
            {

                // Queue empty and still pressed: interval every 10s
                if (_splitterSensor->isPressed())
                {
                    if (_trackSplitterState.lastCountTimeMs + 10000 < now)
                    {
                        _trackSplitterState.queueCount++;
                        _trackSplitterState.lastCountTimeMs = now;
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

        if (_trackBatteryState.nextStatusLogTimeMs < now)
        {
            MLOG_INFO("%s: Battery level: %.2f%%", toString().c_str(), batteryState.batteryPercent);
            _trackBatteryState.nextStatusLogTimeMs = now + 300000; // Log every 5 minutes
        }

        if (batteryState.batteryPercent < 10.0f)
        {
            if (_trackBatteryState.nextCriticalNotificationTimeMs < now)
            {
                _trackBatteryState.nextCriticalNotificationTimeMs = now + 180000; // Play every 3 minutes
                MLOG_WARN("%s: Battery critical (%.2f%%)", toString().c_str(), batteryState.batteryPercent);
                _audio->play(songs::BATTERY_CRITICAL, devices::Hv20tPlayMode::QueueIfPlaying);
            }
        }
        else if (batteryState.batteryPercent < 20.0f)
        {
            if (_trackBatteryState.nextLowNotificationTimeMs < now)
            {
                _trackBatteryState.nextLowNotificationTimeMs = now + 600000; // Play every 10 minutes
                MLOG_WARN("%s: Battery low (%.2f%%)", toString().c_str(), batteryState.batteryPercent);
                _audio->play(songs::BATTERY_LOW, devices::Hv20tPlayMode::QueueIfPlaying);
            }
        }
    }

    void MarbleController::loopConfigError()
    {
        static auto didPlayConfigError = false;
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

    void MarbleController::playStartupSound()
    {
        _trackAudio->playStartupSound();
    }

    void MarbleController::playErrorSound(Hv20tPlayMode mode, std::vector<int> additionalReplaceSongIndexes)
    {
        _trackAudio->playErrorSound(mode, std::move(additionalReplaceSongIndexes));
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
        _trackAudio->playClickSound();
    }

    void MarbleController::playClickOffSound()
    {
        _trackAudio->playClickOffSound();
    }

    void MarbleController::playButtonDown(std::vector<int> additionalReplaceSongIndexes)
    {
        _trackAudio->playButtonDown(std::move(additionalReplaceSongIndexes));
    }

    void MarbleController::playButtonUp(std::vector<int> additionalReplaceSongIndexes)
    {
        _trackAudio->playButtonUp(std::move(additionalReplaceSongIndexes));
    }

    void MarbleController::playButtonClick(std::vector<int> additionalReplaceSongIndexes)
    {
        _trackAudio->playButtonClick(std::move(additionalReplaceSongIndexes));
    }

    void MarbleController::playButtonCountClick(int count, std::vector<int> additionalReplaceSongIndexes)
    {
        _trackAudio->playButtonCountClick(count, std::move(additionalReplaceSongIndexes));
    }

    void MarbleController::onWheelStateChange(void *statePtr)
    {
        auto *wheelState = static_cast<devices::WheelState *>(statePtr);
        if (!wheelState)
        {
            return;
        }

        // * -> CALIBRATING
        if (_trackWheelState.previousState != devices::WheelStateEnum::CALIBRATING &&
            wheelState->state == devices::WheelStateEnum::CALIBRATING)
        {
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_START);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_END);
            _audio->play(songs::WHEEL_CALIBRATION_START, devices::Hv20tPlayMode::QueueIfPlaying);
        }

        // ERROR -> *
        // If error is gone, remove queued error songs
        if (_trackWheelState.previousState == devices::WheelStateEnum::ERROR &&
            wheelState->state != devices::WheelStateEnum::ERROR)
        {
            // Don't play error that are not active anymore
            _audio->removeFromQueue(songs::WHEEL_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND);
            _audio->removeFromQueue(songs::WHEEL_UNEXPECTED_ZERO_TRIGGER);

            _trackWheelState.isPressedDuringError = false;
        }

        // * -> ERROR
        if (_trackWheelState.previousState != devices::WheelStateEnum::ERROR &&
            wheelState->state == devices::WheelStateEnum::ERROR)
        {
            playWheelError(_wheel->getErrorCode());
        }

        // CALIBRATING -> IDLE
        if (_trackWheelState.previousState == devices::WheelStateEnum::CALIBRATING &&
            wheelState->state == devices::WheelStateEnum::IDLE)
        {
            _audio->play(songs::NOTIFICATION, devices::Hv20tPlayMode::QueueIfPlaying);
            _audio->play(songs::WHEEL_CALIBRATION_END, devices::Hv20tPlayMode::QueueIfPlaying);
        }
        _trackWheelState.previousState = wheelState->state;
    }

} // namespace devices
