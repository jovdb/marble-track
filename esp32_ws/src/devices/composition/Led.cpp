/**
 * @file Led.cpp
 * @brief Simple LED implementation using DeviceBase, ConfigMixin, and StateMixin
 */

#include "devices/composition/Led.h"
#include "Logging.h"

namespace composition
{

    Led::Led(const String &id)
        : DeviceBase(id, "led")
    {
    }

    void Led::setup()
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured (pin = -1)", toString().c_str());
            return;
        }

        pinMode(_config.pin, OUTPUT);
        digitalWrite(_config.pin, LOW);
        MLOG_INFO("%s: Setup on pin %d", toString().c_str(), _config.pin);
    }

    std::vector<int> Led::getPins() const
    {
        return {_config.pin};
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

        digitalWrite(_config.pin, HIGH);
        MLOG_INFO("%s: Blinking with on=%lums, off=%lums", toString().c_str(), onTime, offTime);

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    void Led::loop()
    {
        if (_config.pin == -1 || _state.mode != "BLINKING")
            return;

        // Calculate total blink cycle time
        unsigned long cycle = _state.blinkOnTime + _state.blinkOffTime;

        // Use modulo to find position in cycle (0 to cycle-1)
        // This ensures all LEDs using the same timings blink in sync
        unsigned long position = millis() % cycle;

        // LED should be ON if position is within the ON time
        bool shouldBeOn = position < _state.blinkOnTime;

        // Only update GPIO if state changed (check against current mode state)
        bool isCurrentlyOn = (_state.mode == "ON");
        if (isCurrentlyOn != shouldBeOn)
        {
            _state.mode = shouldBeOn ? "ON" : "OFF";
            digitalWrite(_config.pin, shouldBeOn ? HIGH : LOW);
            MLOG_INFO("%s: Blink state changed to %s", toString().c_str(), shouldBeOn ? "ON" : "OFF");

            // Notify subscribers of state change
            notifyStateChanged();
        }
    }

    // Controllable mixin temporarily removed; state JSON and control handlers disabled
}

// namespace composition
