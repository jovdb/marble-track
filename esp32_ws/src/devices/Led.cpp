/**
 * @file Led.cpp
 * @brief Simple LED implementation using DeviceBase, ConfigMixin, and StateMixin
 */

#include "devices/Led.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{
    static bool _isPrevBlinkingOn = false;

    Led::Led(const String &id)
        : DeviceBase(id, "led")
    {
    }

    void Led::setup()
    {
        DeviceBase::setup();

        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured (pin = -1)", toString().c_str());
            return;
        }

        // Set the device name
        setName(_config.name);

        pinMode(_config.pin, OUTPUT);
        MLOG_DEBUG("%s: Setup on pin %d", toString().c_str(), _config.pin);

        // Apply initial state
        if (_config.initialState == "ON")
        {
            set(true);
        }
        else if (_config.initialState == "BLINKING")
        {
            blink(_state.blinkOnTime, _state.blinkOffTime);
        }
        else
        {
            set(false);
        }
    }

    std::vector<int> Led::getPins() const
    {
        if (_config.pin >= 0)
            return {_config.pin};
        return {};
    }

    bool Led::set(bool value)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        // Update state
        _state.mode = value ? "ON" : "OFF";

        digitalWrite(_config.pin, value ? HIGH : LOW);
        MLOG_INFO("%s: Set to %s", toString().c_str(), value ? "ON" : "OFF");

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    bool Led::blink(unsigned long onTime, unsigned long offTime)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        // Update state
        _state.mode = "BLINKING";
        _state.blinkOnTime = onTime;
        _state.blinkOffTime = offTime;

        // Pin set by loop()
        MLOG_INFO("%s: Blinking with on=%lums, off=%lums", toString().c_str(), onTime, offTime);

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    void Led::loop()
    {
        DeviceBase::loop();

        // -1: UnSet, 0: OFF: 1: ON
        static int _isPrevBlinkingOn = -1;

        if (_config.pin == -1 || _state.mode != "BLINKING")
        {
            _isPrevBlinkingOn = -1;
            return;
        }

        // Calculate total blink cycle time
        unsigned long cycle = _state.blinkOnTime + _state.blinkOffTime;

        // Use modulo to find position in cycle (0 to cycle-1)
        // This ensures all LEDs using the same timings blink in sync
        unsigned long position = millis() % cycle;

        // LED should be ON if position is within the ON time
        bool shouldBeOn = position < _state.blinkOnTime;

        // Only update GPIO if state changed (check against current mode state)
        if ((shouldBeOn && _isPrevBlinkingOn != 1) || (!shouldBeOn && _isPrevBlinkingOn != 0))
        {
            _isPrevBlinkingOn = shouldBeOn ? 1 : 0;
            digitalWrite(_config.pin, shouldBeOn ? HIGH : LOW);
        }
    }

    void Led::addStateToJson(JsonDocument &doc)
    {
        doc["mode"] = _state.mode;
        doc["blinkOnTime"] = _state.blinkOnTime;
        doc["blinkOffTime"] = _state.blinkOffTime;
    }

    bool Led::control(const String &action, JsonObject *args)
    {
        if (action == "set")
        {
            if (!args || !(*args)["value"].is<bool>())
            {
                MLOG_WARN("%s: 'set' action requires 'value' argument", toString().c_str());
                return false;
            }
            bool value = (*args)["value"].as<bool>();
            return set(value);
        }
        else if (action == "blink")
        {
            unsigned long onTime = 500;
            unsigned long offTime = 500;

            if (args)
            {
                if ((*args)["onTime"].is<unsigned long>())
                    onTime = (*args)["onTime"].as<unsigned long>();
                if ((*args)["offTime"].is<unsigned long>())
                    offTime = (*args)["offTime"].as<unsigned long>();
            }

            return blink(onTime, offTime);
        }
        else
        {
            MLOG_WARN("%s: Unknown control action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Led::jsonToConfig(const JsonDocument &config)
    {
        if (config["pin"].is<int>())
        {
            _config.pin = config["pin"].as<int>();
        }
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
        if (config["initialState"].is<String>())
        {
            _config.initialState = config["initialState"].as<String>();
        }
    }

    void Led::configToJson(JsonDocument &doc)
    {
        doc["pin"] = _config.pin;
        doc["name"] = _config.name;
        doc["initialState"] = _config.initialState;
    }

} // namespace devices
