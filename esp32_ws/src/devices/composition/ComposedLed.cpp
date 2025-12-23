/**
 * @file ComposedLed.cpp
 * @brief Implementation of LED device using composition pattern
 */

#include "devices/composition/ComposedLed.h"
#include "Logging.h"

ComposedLed::ComposedLed(const String &id)
    : DeviceBase(id, "led")
{
}

// =============================================================================
// DeviceBase overrides
// =============================================================================

void ComposedLed::setup()
{
    if (_pin != -1)
    {
        pinMode(_pin, OUTPUT);
        applyInitialState();

        // Start RTOS task
        startTask(_id, 2048, 1, 1);
    }
    else
    {
        MLOG_WARN("ComposedLed [%s]: Pin not configured", _id.c_str());
    }
}

void ComposedLed::loop()
{
    // Most work is done in the RTOS task
    // This could be used for non-task-based polling if needed
}

std::vector<int> ComposedLed::getPins() const
{
    if (_pin == -1)
        return {};
    return {_pin};
}

// =============================================================================
// RtosMixin: RTOS task implementation
// =============================================================================

void ComposedLed::task()
{
    while (true)
    {
        if (_pin == -1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        Mode currentMode = _targetMode.load();
        bool targetState = _targetState.load();

        if (currentMode == Mode::BLINKING)
        {
            unsigned long waitTime = _isOn
                                         ? _targetBlinkOnTime.load()
                                         : _targetBlinkOffTime.load();

            // Wait for notification or timeout
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitTime)) > 0)
            {
                // Notified - loop will pick up new state
                continue;
            }

            // Timeout: toggle LED
            _isOn = !_isOn;
            digitalWrite(_pin, _isOn ? HIGH : LOW);
        }
        else
        {
            // Static ON or OFF
            digitalWrite(_pin, targetState ? HIGH : LOW);
            _isOn = targetState;

            // Wait indefinitely for notification
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

// =============================================================================
// SaveableMixin: config persistence
// =============================================================================

void ComposedLed::loadConfigFromJson(const JsonDocument &config)
{
    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }

    if (config["initialState"].is<String>())
    {
        _initialMode = modeFromString(config["initialState"].as<String>());
    }

    if (config["blinkOnTime"].is<unsigned long>())
    {
        _blinkOnTime = config["blinkOnTime"].as<unsigned long>();
        _targetBlinkOnTime = _blinkOnTime;
    }

    if (config["blinkOffTime"].is<unsigned long>())
    {
        _blinkOffTime = config["blinkOffTime"].as<unsigned long>();
        _targetBlinkOffTime = _blinkOffTime;
    }
}

void ComposedLed::saveConfigToJson(JsonDocument &doc) const
{
    doc["pin"] = _pin;
    doc["initialState"] = modeToString(_initialMode);
    doc["blinkOnTime"] = _blinkOnTime;
    doc["blinkOffTime"] = _blinkOffTime;
}

// =============================================================================
// ControllableMixin: control and state
// =============================================================================

bool ComposedLed::handleControl(const String &action, JsonObject *args)
{
    if (action == "set")
    {
        if (!args || !(*args)["value"].is<bool>())
        {
            MLOG_ERROR("ComposedLed [%s]: Invalid 'set' payload", _id.c_str());
            return false;
        }
        return set((*args)["value"].as<bool>());
    }

    if (action == "toggle")
    {
        toggle();
        return true;
    }

    if (action == "blink")
    {
        unsigned long onTime = 500;
        unsigned long offTime = 500;
        if (args)
        {
            if ((*args)["onTime"].is<unsigned long>())
            {
                onTime = (*args)["onTime"].as<unsigned long>();
            }
            if ((*args)["offTime"].is<unsigned long>())
            {
                offTime = (*args)["offTime"].as<unsigned long>();
            }
        }
        return blink(onTime, offTime);
    }

    MLOG_WARN("ComposedLed [%s]: Unknown action: %s", _id.c_str(), action.c_str());
    return false;
}

void ComposedLed::addStateToJson(JsonDocument &doc) const
{
    doc["mode"] = modeToString(_mode);
    doc["isOn"] = _isOn;
}

// =============================================================================
// LED-specific operations
// =============================================================================

bool ComposedLed::set(bool state)
{
    if (_pin == -1)
        return false;

    // Avoid redundant updates
    if (_mode == Mode::OFF && !state)
        return true;
    if (_mode == Mode::ON && state)
        return true;

    _mode = state ? Mode::ON : Mode::OFF;
    _targetMode = _mode;
    _targetState = state;
    _isOn = state;

    // Write immediately (task will also maintain state)
    digitalWrite(_pin, state ? HIGH : LOW);

    // Wake task
    notifyTask();

    // Notify state change (WebSocket)
    notifyStateChange();

    // Emit to internal subscribers
    emitStateChange(this);

    return true;
}

bool ComposedLed::blink(unsigned long onTime, unsigned long offTime)
{
    if (_pin == -1)
    {
        MLOG_WARN("ComposedLed [%s]: Pin not configured", _id.c_str());
        return false;
    }

    _mode = Mode::BLINKING;
    _targetMode = Mode::BLINKING;
    _blinkOnTime = onTime;
    _blinkOffTime = offTime;
    _targetBlinkOnTime = onTime;
    _targetBlinkOffTime = offTime;

    notifyTask();
    notifyStateChange();
    emitStateChange(this);

    return true;
}

void ComposedLed::toggle()
{
    set(!_isOn);
}

// =============================================================================
// Private helpers
// =============================================================================

void ComposedLed::applyInitialState()
{
    switch (_initialMode)
    {
    case Mode::ON:
        set(true);
        break;
    case Mode::BLINKING:
        blink(_blinkOnTime, _blinkOffTime);
        break;
    case Mode::OFF:
    default:
        set(false);
        break;
    }
}

ComposedLed::Mode ComposedLed::modeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("ON"))
        return Mode::ON;
    if (value.equalsIgnoreCase("BLINKING"))
        return Mode::BLINKING;
    return Mode::OFF;
}

String ComposedLed::modeToString(Mode mode) const
{
    switch (mode)
    {
    case Mode::ON:
        return "ON";
    case Mode::BLINKING:
        return "BLINKING";
    default:
        return "OFF";
    }
}
