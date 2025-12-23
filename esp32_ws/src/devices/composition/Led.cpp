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
    _isOn = value;
    _mode = value ? Mode::ON : Mode::OFF;
    digitalWrite(_pin, value ? HIGH : LOW);
    MLOG_INFO("%s: Set to %s", toString().c_str(), value ? "ON" : "OFF");
    return true;
}

bool Led::blink(unsigned long onTime, unsigned long offTime)
{
    _mode = Mode::BLINKING;
    _blinkOnTime = onTime;
    _blinkOffTime = offTime;
    _isOn = true;  // Start in ON state
    digitalWrite(_pin, HIGH);
    MLOG_INFO("%s: Blinking with on=%lums, off=%lums", toString().c_str(), onTime, offTime);
    return true;
}

void Led::loop()
{
    if (_mode != Mode::BLINKING)
        return;

    // Calculate total blink cycle time
    unsigned long cycle = _blinkOnTime + _blinkOffTime;
    
    // Use modulo to find position in cycle (0 to cycle-1)
    // This ensures all LEDs using the same timings blink in sync
    unsigned long position = millis() % cycle;
    
    // LED should be ON if position is within the ON time
    bool shouldBeOn = position < _blinkOnTime;
    
    // Only update GPIO if state changed
    if (_isOn != shouldBeOn)
    {
        _isOn = shouldBeOn;
        digitalWrite(_pin, _isOn ? HIGH : LOW);
    }
}

} // namespace composition
