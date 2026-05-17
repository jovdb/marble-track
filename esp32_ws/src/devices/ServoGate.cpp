/**
 * @file ServoGate.cpp
 * @brief ServoGate composite device implementation
 */

#include "devices/ServoGate.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    namespace servo_gate_timing
    {
        static constexpr unsigned long HoldToFillQueueMs = 2000UL;
    }

    ServoGate::ServoGate(const String &id)
        : Device(id, "servogate")
    {
        _button = new Button(getId() + "-button");
        auto btnCfg = _button->getConfig();
        btnCfg.name = "ServoGate Button";
        _button->setConfig(btnCfg);
        addChild(_button);

        _servo = new Servo(getId() + "-servo");
        auto srvCfg = _servo->getConfig();
        srvCfg.name = "ServoGate Servo";
        _servo->setConfig(srvCfg);
        addChild(_servo);
    }

    void ServoGate::setup()
    {
        Device::setup();

        setName(_config.name);

        _fsm = ServoGateFsmState::IDLE;
        _buttonPressStartMs = 0;
        _holdQueueFillApplied = false;
        _timerStart = 0;
        _timerDuration = 0;
        _state.gateState = "Idle";
        _state.queueCount = 0;
        _state.pulseCount = 0;
        notifyStateChanged();

        MLOG_INFO("%s: Setup complete", toString().c_str());
    }

    void ServoGate::teardown()
    {
        Device::teardown();
        _fsm = ServoGateFsmState::IDLE;

        // Stop sending PWM to allow free rotation when idle
        if (_servo)
            _servo->disable();

        _buttonPressStartMs = 0;
        _holdQueueFillApplied = false;
    }

    bool ServoGate::isTimerExpired() const
    {
        return (millis() - _timerStart) >= _timerDuration;
    }

    void ServoGate::startTimer(uint32_t durationMs)
    {
        _timerStart = millis();
        _timerDuration = durationMs;
    }

    void ServoGate::startCycle()
    {
        Device::clearError();
        _fsm = ServoGateFsmState::WAIT_OPEN;
        startTimer(_config.openDelayMs);
        _state.gateState = fsmStateToString(_fsm);
        notifyStateChanged();
        MLOG_INFO("%s: Starting cycle in %lu ms (queue=%d)",
                  toString().c_str(), static_cast<unsigned long>(_config.openDelayMs), _state.queueCount);
    }

    void ServoGate::loop()
    {
        // Calls loop() on children (button and servo) first
        Device::loop();

        // Check if the gate servo is in error; skip button/FSM processing if so
        if (checkChildErrors())
            return;

        if (_button != nullptr)
        {
            const auto &btnState = _button->getState();
            if (btnState.isPressed)
            {
                if (btnState.isPressedChanged)
                {
                    _buttonPressStartMs = millis();
                    _holdQueueFillApplied = false;

                    // New click: add one to queue
                    if (_state.queueCount < _config.fullQueueCount)
                    {
                        _state.queueCount++;
                        // MLOG_INFO("%s: Button clicked, queue=%d", toString().c_str(), _state.queueCount);
                        notifyStateChanged();
                    }
                }
                else
                {
                    // Still held: after 2s continuous hold, fill queue once.
                    if (!_holdQueueFillApplied &&
                        _state.queueCount < _config.fullQueueCount &&
                        (millis() - _buttonPressStartMs) >= servo_gate_timing::HoldToFillQueueMs)
                    {
                        _state.queueCount = _config.fullQueueCount;
                        _holdQueueFillApplied = true;
                        MLOG_INFO("%s: Button held, queue filled to %d", toString().c_str(), _state.queueCount);
                        notifyStateChanged();
                    }
                }
            }
            else if (btnState.isPressedChanged)
            {
                _buttonPressStartMs = 0;
                _holdQueueFillApplied = false;
            }
        }

        switch (_fsm)
        {
        case ServoGateFsmState::IDLE:
            if (_state.queueCount > 0)
            {
                startCycle();
            }
            break;

        case ServoGateFsmState::WAIT_OPEN:
            if (isTimerExpired())
            {
                const uint32_t servoDuration = _servo ? _servo->getConfig().defaultDurationInMs : 500;
                if (_servo)
                    _servo->setValue(1.0f, static_cast<int>(servoDuration));
                startTimer(servoDuration);
                _fsm = ServoGateFsmState::OPENING;
                _state.gateState = fsmStateToString(_fsm);
                notifyStateChanged();
                // MLOG_INFO("%s: Opening servo over %lu ms", toString().c_str(), static_cast<unsigned long>(servoDuration));
            }
            break;

        case ServoGateFsmState::OPENING:
            if (isTimerExpired())
            {
                startTimer(_config.closeDelayMs);
                _fsm = ServoGateFsmState::WAIT_CLOSE;
                _state.gateState = fsmStateToString(_fsm);
                notifyStateChanged();
                // MLOG_INFO("%s: Servo open, holding %lu ms before closing", toString().c_str(), static_cast<unsigned long>(_config.closeDelayMs));
            }
            break;

        case ServoGateFsmState::WAIT_CLOSE:
            if (isTimerExpired())
            {
                const uint32_t servoDuration = _servo ? _servo->getConfig().defaultDurationInMs : 500;
                if (_servo)
                    _servo->setValue(0.0f, static_cast<int>(servoDuration));
                startTimer(servoDuration);
                _fsm = ServoGateFsmState::CLOSING;
                _state.gateState = fsmStateToString(_fsm);
                notifyStateChanged();
                // MLOG_INFO("%s: Closing servo over %lu ms", toString().c_str(), static_cast<unsigned long>(servoDuration));
            }
            break;

        case ServoGateFsmState::CLOSING:
            if (isTimerExpired())
            {
                _state.queueCount--;
                _state.pulseCount++;
                // MLOG_INFO("%s: Cycle complete (total pulses=%d), queue=%d", toString().c_str(), _state.pulseCount, _state.queueCount);

                if (_state.queueCount > 0)
                {
                    startTimer(_config.betweenDelayMs);
                    _fsm = ServoGateFsmState::BETWEEN;
                    _state.gateState = fsmStateToString(_fsm);
                }
                else
                {
                    _fsm = ServoGateFsmState::IDLE;
                    _state.gateState = fsmStateToString(_fsm);
                    // Stop sending PWM to allow free rotation when idle
                    if (_servo)
                        _servo->disable();
                }
                notifyStateChanged();
            }
            break;

        case ServoGateFsmState::BETWEEN:
            if (isTimerExpired())
            {
                startCycle();
            }
            break;
        }
    }

    void ServoGate::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["gateState"] = _state.gateState;
        doc["queueCount"] = _state.queueCount;
        doc["pulseCount"] = _state.pulseCount;
    }

    bool ServoGate::control(const String &action, JsonObject *args)
    {
        if (action == "trigger")
        {
            if (_state.queueCount < _config.fullQueueCount)
            {
                _state.queueCount++;
                notifyStateChanged();
                MLOG_INFO("%s: Manual trigger, queue=%d", toString().c_str(), _state.queueCount);
                return true;
            }
            MLOG_WARN("%s: Queue full (%d), trigger ignored", toString().c_str(), _state.queueCount);
            return false;
        }

        MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
        return false;
    }

    void ServoGate::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["openDelayMs"].is<uint32_t>())
            _config.openDelayMs = config["openDelayMs"].as<uint32_t>();
        else if (config["openDelayMs"].is<int>())
            _config.openDelayMs = static_cast<uint32_t>(config["openDelayMs"].as<int>());
        if (config["closeDelayMs"].is<uint32_t>())
            _config.closeDelayMs = config["closeDelayMs"].as<uint32_t>();
        else if (config["closeDelayMs"].is<int>())
            _config.closeDelayMs = static_cast<uint32_t>(config["closeDelayMs"].as<int>());
        if (config["betweenDelayMs"].is<uint32_t>())
            _config.betweenDelayMs = config["betweenDelayMs"].as<uint32_t>();
        else if (config["betweenDelayMs"].is<int>())
            _config.betweenDelayMs = static_cast<uint32_t>(config["betweenDelayMs"].as<int>());
        if (config["fullQueueCount"].is<int>())
            _config.fullQueueCount = config["fullQueueCount"].as<int>();
    }

    void ServoGate::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["openDelayMs"] = _config.openDelayMs;
        doc["closeDelayMs"] = _config.closeDelayMs;
        doc["betweenDelayMs"] = _config.betweenDelayMs;
        doc["fullQueueCount"] = _config.fullQueueCount;
    }

    String ServoGate::fsmStateToString(ServoGateFsmState state) const
    {
        switch (state)
        {
        case ServoGateFsmState::IDLE:
            return "Idle";
        case ServoGateFsmState::WAIT_OPEN:
            return "WaitOpen";
        case ServoGateFsmState::OPENING:
            return "Opening";
        case ServoGateFsmState::WAIT_CLOSE:
            return "WaitClose";
        case ServoGateFsmState::CLOSING:
            return "Closing";
        case ServoGateFsmState::BETWEEN:
            return "Between";
        default:
            return "Unknown";
        }
    }

    bool ServoGate::checkChildErrors()
    {
        const bool servoInError = _servo != nullptr && _servo->getState().state == ServoStateEnum::ERROR;

        if (servoInError && !_childErrorActive)
        {
            // Servo just entered error — block the gate
            MLOG_ERROR("%s: required child 'servo' has an error: %s", toString().c_str(), _servo->getErrorMessage().c_str());
            _childErrorActive = true;
            _state.queueCount = 0;
            _fsm = ServoGateFsmState::IDLE;
            _state.gateState = fsmStateToString(_fsm);
            // Stop sending PWM to allow free rotation when idle
            if (_servo)
                _servo->disable();
            notifyStateChanged();
        }
        else if (!servoInError && _childErrorActive)
        {
            // Servo error has cleared — resume
            MLOG_INFO("%s: Child servo error resolved, resuming", toString().c_str());
            _childErrorActive = false;
            Device::clearError();
            notifyStateChanged();
        }

        return _childErrorActive;
    }

} // namespace devices
