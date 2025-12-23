/**
 * @file Led.cpp
 * @brief Simple LED implementation using only DeviceBase
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
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        MLOG_INFO("%s: Setup on pin %d", toString().c_str(), _pin);
    }

    std::vector<int> Led::getPins() const
    {
        return {_pin};
    }

    bool Led::set(bool value)
    {
        // Update public state
        state.mode = value ? "ON" : "OFF";

        digitalWrite(_pin, value ? HIGH : LOW);
        MLOG_INFO("%s: Set to %s", toString().c_str(), value ? "ON" : "OFF");

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    bool Led::blink(unsigned long onTime, unsigned long offTime)
    {
        // Update public state
        state.mode = "BLINKING";
        state.blinkOnTime = onTime;
        state.blinkOffTime = offTime;

        digitalWrite(_pin, HIGH);
        MLOG_INFO("%s: Blinking with on=%lums, off=%lums", toString().c_str(), onTime, offTime);

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    void Led::loop()
    {
        if (state.mode != "BLINKING")
            return;

        // Calculate total blink cycle time
        unsigned long cycle = state.blinkOnTime + state.blinkOffTime;

        // Use modulo to find position in cycle (0 to cycle-1)
        // This ensures all LEDs using the same timings blink in sync
        unsigned long position = millis() % cycle;

        // LED should be ON if position is within the ON time
        bool shouldBeOn = position < state.blinkOnTime;

        // Only update GPIO if state changed (check against current mode state)
        bool isCurrentlyOn = (state.mode == "ON");
        if (isCurrentlyOn != shouldBeOn)
        {
            state.mode = shouldBeOn ? "ON" : "OFF";
            digitalWrite(_pin, shouldBeOn ? HIGH : LOW);
            MLOG_INFO("%s: Blink state changed to %s", toString().c_str(), shouldBeOn ? "ON" : "OFF");

            // Notify subscribers of state change
            notifyStateChanged();
        }
    }

} // namespace composition
