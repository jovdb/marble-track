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

        _state.state = LauncherStateEnum::INIT;
        _state.isBallLoaded = false;
        _state.isBallWaiting = false;
        _timerStart = 0;
        _timerDuration = 0;
        _isBallLoadedAtMoveStart = false;
        _pendingLoadDown = false;
        _isInitMove = false;

        if (_servo && _servo->getPins().empty())
        {
            setError("LAUNCHER_CONFIG_ERROR", "No pins configured for servo");
            MLOG_WARN("%s: No pins configured for servo", toString().c_str());
        }

        notifyStateChanged();
        MLOG_INFO("%s: Setup complete", toString().c_str());
    }

    void Launcher::teardown()
    {
        Device::teardown();
        _state.state = LauncherStateEnum::INIT;
        _pendingLoadDown = false;
        _isInitMove = false;
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

        // Sync isBallWaiting from button; also set isBallLoaded when sensor is pressed
        const bool newBallWaiting = _button ? _button->isPressed() : false;
        bool stateChanged = false;
        if (newBallWaiting != _state.isBallWaiting)
        {
            _state.isBallWaiting = newBallWaiting;
            stateChanged = true;
        }
        if (newBallWaiting && !_state.isBallLoaded)
        {
            _state.isBallLoaded = true;
            stateChanged = true;
        }
        if (stateChanged)
            notifyStateChanged();

        // Auto-start load when a ball is waiting but none is loaded and arm is idle
        if (_state.isBallWaiting && !_state.isBallLoaded &&
            (_state.state == LauncherStateEnum::DOWN || _state.state == LauncherStateEnum::INIT))
        {
            load();
        }

        // FSM: advance state when current motion timer expires
        switch (_state.state)
        {
        case LauncherStateEnum::MOVING_UP:
            if (isTimerExpired())
            {
                _state.state = LauncherStateEnum::UP;

                if (_pendingLoadDown)
                {
                    // Load cycle: ball (if waiting) is now on the arm; move arm down slowly
                    _pendingLoadDown = false;
                    _isBallLoadedAtMoveStart = _state.isBallWaiting;
                    if (_servo)
                        _servo->setValue(0.0f, static_cast<int>(_config.loadTimeMs));
                    startTimer(_config.loadTimeMs);
                    _state.state = LauncherStateEnum::MOVING_DOWN;
                }

                notifyStateChanged();
            }
            break;

        case LauncherStateEnum::MOVING_DOWN:
            if (isTimerExpired())
            {
                _state.state = LauncherStateEnum::DOWN;

                if (_isInitMove)
                {
                    // init() assumes a ball is already loaded
                    _state.isBallLoaded = true;
                    _isInitMove = false;
                }
                else
                {
                    // Ball is loaded if it was waiting when arm reached the UP position
                    _state.isBallLoaded = _isBallLoadedAtMoveStart;
                }

                notifyStateChanged();
            }
            break;

        default:
            break;
        }
    }

    // ---------------------------------------------------------------------------
    // Device functions
    // ---------------------------------------------------------------------------

    bool Launcher::init()
    {
        // Move arm slowly down and assume a ball is waiting to be launched
        clearError();
        if (_servo)
            _servo->setValue(0.0f, static_cast<int>(_config.loadTimeMs));
        startTimer(_config.loadTimeMs);
        _state.state = LauncherStateEnum::MOVING_DOWN;
        _state.isBallLoaded = false;
        _pendingLoadDown = false;
        _isInitMove = true;
        notifyStateChanged();
        MLOG_INFO("%s: init – moving arm down over %lu ms", toString().c_str(),
                  static_cast<unsigned long>(_config.loadTimeMs));
        return true;
    }

    bool Launcher::load()
    {
        if (_state.state == LauncherStateEnum::MOVING_UP ||
            _state.state == LauncherStateEnum::MOVING_DOWN)
        {
            MLOG_WARN("%s: load ignored – arm is already moving", toString().c_str());
            return false;
        }

        if (_state.state == LauncherStateEnum::UP)
        {
            // Arm is already up; move down so the ball rolls to the launch end
            _isBallLoadedAtMoveStart = _state.isBallWaiting;
            if (_servo)
                _servo->setValue(0.0f, static_cast<int>(_config.loadTimeMs));
            startTimer(_config.loadTimeMs);
            _state.state = LauncherStateEnum::MOVING_DOWN;
            _pendingLoadDown = false;
            notifyStateChanged();
            MLOG_INFO("%s: load (already up) – moving arm down over %lu ms", toString().c_str(),
                      static_cast<unsigned long>(_config.loadTimeMs));
            return true;
        }

        // Arm is DOWN or INIT: move up slowly, then automatically move down
        if (_servo)
            _servo->setValue(1.0f, static_cast<int>(_config.loadTimeMs));
        startTimer(_config.loadTimeMs);
        _state.state = LauncherStateEnum::MOVING_UP;
        _pendingLoadDown = true;
        _isInitMove = false;
        notifyStateChanged();
        MLOG_INFO("%s: load – moving arm up over %lu ms then down", toString().c_str(),
                  static_cast<unsigned long>(_config.loadTimeMs));
        return true;
    }

    bool Launcher::launch()
    {
        if (_state.state == LauncherStateEnum::MOVING_UP)
        {
            MLOG_WARN("%s: launch ignored – arm is already moving up", toString().c_str());
            return false;
        }

        // Swing arm up fast – this throws the ball
        _state.isBallLoaded = false;
        if (_servo)
            _servo->setValue(1.0f, static_cast<int>(_config.launchTimeMs));
        startTimer(_config.launchTimeMs);
        _state.state = LauncherStateEnum::MOVING_UP;
        _pendingLoadDown = false;
        _isInitMove = false;
        notifyStateChanged();
        MLOG_INFO("%s: launch – swinging arm up over %lu ms", toString().c_str(),
                  static_cast<unsigned long>(_config.launchTimeMs));
        return true;
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
        case LauncherStateEnum::INIT:
            return "Init";
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
