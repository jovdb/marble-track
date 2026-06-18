/**
 * @file Launcher.cpp
 * @brief Launcher composite device implementation
 *
 * Arm position convention:
 *   servo value 1.0f = UP   (arm raised; ball from queue can roll on)
 *   servo value 0.0f = DOWN (arm lowered; ball rolls to end for launch)
 */

#include "devices/Launcher.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    Launcher::Launcher(const String &id)
        : Device(id, "launcher")
    {
        _button = new Button(getId() + "-button");
        auto btnCfg = _button->getConfig();
        btnCfg.name = "Launcher Ball Sensor";
        _button->setConfig(btnCfg);
        addChild(_button);

        _servo = new Servo(getId() + "-servo");
        auto srvCfg = _servo->getConfig();
        srvCfg.name = "Launcher Arm Servo";
        _servo->setConfig(srvCfg);
        addChild(_servo);
    }

    void Launcher::setup()
    {
        Device::setup();

        setName(_config.name);

        _state.state = LauncherStateEnum::UNKNOWN;
        _state.isBallLoaded = true;
        _state.isBallWaiting = false;
        _state.isLoadingStep = 0;
        _state.isLaunchingStep = 0;

        // Delay
        _timerStart = 0;
        _timerDuration = 0;

        if (_servo && _servo->getPins().empty())
        {
            _state.state = LauncherStateEnum::ERROR;
            setError("LAUNCHER_CONFIG_ERROR", "No pins configured for servo");
            MLOG_WARN("%s: No pins configured for servo", toString().c_str());
        }

        notifyStateChanged();
        MLOG_INFO("%s: Setup complete", toString().c_str());
    }

    void Launcher::teardown()
    {
        Device::teardown();
        _state.state = LauncherStateEnum::UNKNOWN;
        _state.isLoadingStep = 0;
        _state.isLaunchingStep = 0;
        _state.isBallLoaded = true;
    }

    bool Launcher::isTimerExpired() const
    {
        return (millis() - _timerStart) >= _timerDuration;
    }

    void Launcher::startTimer(uint32_t durationMs)
    {
        _timerStart = millis();
        _timerDuration = durationMs;
    }

    void Launcher::loop()
    {
        // Update children (button reads pin, servo animates position)
        Device::loop();

        // Sync isBallWaiting
        const bool newBallWaiting = _button ? _button->isPressed() : false;
        if (newBallWaiting != _state.isBallWaiting)
        {
            _state.isBallWaiting = newBallWaiting;
            notifyStateChanged();
        }

        loadLoop();
        launchLoop();

        if (isTimerExpired())
        {
            if (_state.state == LauncherStateEnum::MOVING_UP)
            {
                _state.state = LauncherStateEnum::UP;
                notifyStateChanged();
            }
            else if (_state.state == LauncherStateEnum::MOVING_DOWN)
            {
                _state.state = LauncherStateEnum::DOWN;
                notifyStateChanged();
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Device functions
    // ---------------------------------------------------------------------------

    bool Launcher::init()
    {
        // Move arm slowly down and assume a ball is waiting to be launched
        clearError();

        MLOG_INFO("%s: init – Start loading", toString().c_str());

        // Just move arm down
        moveDown();

        // Assume loaded
        _state.isBallLoaded = true;
        notifyStateChanged();

        return true;
    }

    bool Launcher::moveDown()
    {
        if (!_servo)
            return false;

        if (_servo->setValue(0.0f, static_cast<int>(_config.loadTimeMs)))
        {
            startTimer(_config.loadTimeMs);
            _state.state = LauncherStateEnum::MOVING_DOWN;
            notifyStateChanged();
            return true;
        }
        else
        {
            MLOG_ERROR("%s: Failed to move servo down – check servo", toString().c_str());
            return false;
        }
    }

    bool Launcher::moveUp(int duration)
    {
        if (!_servo)
            return false;

        if (_servo->setValue(1.0f, static_cast<int>(duration)))
        {
            startTimer(duration);
            _state.state = LauncherStateEnum::MOVING_UP;
            notifyStateChanged();
            return true;
        }
        else
        {
            MLOG_ERROR("%s: Failed to move servo up – check servo", toString().c_str());
            return false;
        }
    }

    bool Launcher::launch()
    {
        if (_state.isLoadingStep != 0)
        {
            MLOG_WARN("%s: launch ignored – currently in loading process (step: %i)", toString().c_str(), _state.isLoadingStep);
            return false;
        }
        if (_state.isLaunchingStep != 0)
        {
            MLOG_WARN("%s: launch ignored – already in launching process (step: %i)", toString().c_str(), _state.isLaunchingStep);
            return false;
        }

        _state.isLaunchingStep = 1;
        return true;
    }

    bool Launcher::launchLoop()
    {
        if (_state.isLaunchingStep == 0)
            return false;

        // Move Up
        if (_state.isLaunchingStep == 1)
        {
            MLOG_INFO("%s: Launching started!", toString().c_str());

            if (moveUp(static_cast<int>(_config.launchTimeMs)))
            {
                _state.isLaunchingStep = 2;
                _state.isBallLoaded = _state.isBallWaiting;
                notifyStateChanged();
            }
            else
            {
                MLOG_ERROR("%s: Launch failed", toString().c_str());
                _state.isLaunchingStep = 0;
                _state.state = LauncherStateEnum::ERROR;
                setError("LAUNCH_FAILED", "Launch failed");
                notifyStateChanged();
            }
        }
        // Wait at top
        else if (_state.isLaunchingStep == 2 && _state.state == LauncherStateEnum::UP)
        {
            startTimer(200);
            _state.isLaunchingStep = 3;
        }
        // Move Down
        else if (_state.isLaunchingStep == 3 && isTimerExpired())
        {
            if (moveDown())
            {
                _state.isLaunchingStep = 4;
            }
            else
            {
                MLOG_ERROR("%s: Launch failed", toString().c_str());
                _state.isLaunchingStep = 0;
                _state.state = LauncherStateEnum::ERROR;
                setError("LAUNCH_FAILED", "Launch failed");
                notifyStateChanged();
            }
        }
        // Wait until down
        else if (_state.isLaunchingStep == 4 && _state.state == LauncherStateEnum::DOWN)
        {
            // End
            MLOG_INFO("%s: Launching ended", toString().c_str());
            _state.isLaunchingStep = 0;
            notifyStateChanged();
        }
        return true;
    }

    bool Launcher::load()
    {

        if (_state.isLoadingStep != 0)
        {
            MLOG_WARN("%s: load ignored – already in loading process (step: %i)", toString().c_str(), _state.isLoadingStep);
            return false;
        }

        if (_state.isLaunchingStep != 0)
        {
            MLOG_WARN("%s: load ignored – currently in launching process (step: %i)", toString().c_str(), _state.isLaunchingStep);
            return false;
        }

        _state.isLoadingStep = 1;
        return true;
    }

    bool Launcher::loadLoop()
    {
        if (_state.isLoadingStep == 0)
            return false;

        // Move Up
        if (_state.isLoadingStep == 1)
        {
            MLOG_INFO("%s: Loading started!", toString().c_str());
            if (moveUp(static_cast<int>(_config.loadTimeMs)))
            {
                _state.isLoadingStep = 2;
                _state.isBallLoaded = _state.isBallWaiting;
                notifyStateChanged();
            }
            else
            {
                MLOG_ERROR("%s: Load failed", toString().c_str());
                _state.isLoadingStep = 0;
                _state.state = LauncherStateEnum::ERROR;
                setError("LOAD_FAILED", "Load failed");
                notifyStateChanged();
            }
        }
        // Wait at top
        if (_state.isLoadingStep == 2 && _state.state == LauncherStateEnum::UP)
        {
            startTimer(500);
            _state.isLoadingStep = 3;
        }
        // Move Down
        else if (_state.isLoadingStep == 3 && isTimerExpired())
        {
            if (moveDown())
            {
                _state.isLoadingStep = 4;
            }
            else
            {
                MLOG_ERROR("%s: Load failed", toString().c_str());
                _state.isLoadingStep = 0;
                _state.state = LauncherStateEnum::ERROR;
                setError("LOAD_FAILED", "Load failed");
                notifyStateChanged();
            }
        }
        // Wait until down
        else if (_state.isLoadingStep == 4 && _state.state == LauncherStateEnum::DOWN)
        {
            // End
            MLOG_INFO("%s: Loading ended", toString().c_str());
            _state.isLoadingStep = 0;
            notifyStateChanged();
        }
        return true;
    }

    void Launcher::setErrorState(LauncherErrorCode errorCode, const String &errorMessage)
    {
        _state.state = LauncherStateEnum::ERROR;
        // if (_servo)
        // _servo->disable();
        Device::setError(errorCodeToString(errorCode), errorMessage);
    }

    String Launcher::errorCodeToString(LauncherErrorCode errorCode) const
    {
        switch (errorCode)
        {
        case LauncherErrorCode::None:
            return "None";
        case LauncherErrorCode::CalibrationZeroNotFound:
            return "CalibrationZeroNotFound";
        case LauncherErrorCode::CalibrationSecondZeroNotFound:
        default:
            return "Unknown";
        }
    }

    // ---------------------------------------------------------------------------
    // ControllableMixin
    // ---------------------------------------------------------------------------

    void Launcher::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["state"] = stateToString(_state.state);
        doc["isBallLoaded"] = _state.isBallLoaded;
        doc["isBallWaiting"] = _state.isBallWaiting;
    }

    bool Launcher::control(const String &action, JsonObject * /*args*/)
    {
        if (action == "init")
            return init();
        if (action == "load")
            return load();
        if (action == "launch")
            return launch();

        MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
        return false;
    }

    // ---------------------------------------------------------------------------
    // SerializableMixin
    // ---------------------------------------------------------------------------

    void Launcher::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["loadTimeMs"].is<uint32_t>())
            _config.loadTimeMs = config["loadTimeMs"].as<uint32_t>();
        else if (config["loadTimeMs"].is<int>())
            _config.loadTimeMs = static_cast<uint32_t>(config["loadTimeMs"].as<int>());
        if (config["launchTimeMs"].is<uint32_t>())
            _config.launchTimeMs = config["launchTimeMs"].as<uint32_t>();
        else if (config["launchTimeMs"].is<int>())
            _config.launchTimeMs = static_cast<uint32_t>(config["launchTimeMs"].as<int>());
    }

    void Launcher::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["loadTimeMs"] = _config.loadTimeMs;
        doc["launchTimeMs"] = _config.launchTimeMs;
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    String Launcher::stateToString(LauncherStateEnum state) const
    {
        switch (state)
        {
        case LauncherStateEnum::UNKNOWN:
            return "Unknown";
        case LauncherStateEnum::ERROR:
            return "Error";
        case LauncherStateEnum::UP:
            return "Up";
        case LauncherStateEnum::MOVING_UP:
            return "MovingUp";
        case LauncherStateEnum::DOWN:
            return "Down";
        case LauncherStateEnum::MOVING_DOWN:
            return "MovingDown";
        default:
            return "Unknown";
        }
    }

} // namespace devices
