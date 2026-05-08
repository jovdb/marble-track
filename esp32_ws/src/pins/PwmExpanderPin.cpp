/**
 * @file PwmExpanderPin.cpp
 * @brief PCA9685 PWM expander pin implementation
 */

#include "pins/PwmExpanderPin.h"
#include "Logging.h"

namespace pins
{
    PwmExpanderPin::PwmExpanderPin(Adafruit_PWMServoDriver *driver, const String &expanderId)
        : _driver(driver), _expanderId(expanderId), _channel(-1), _isSetup(false)
    {
    }

    bool PwmExpanderPin::setup(int pinNumber, PinMode /*mode*/)
    {
        if (!_driver)
        {
            MLOG_ERROR("PwmExpanderPin: driver is null");
            return false;
        }
        if (pinNumber < 0 || pinNumber > 15)
        {
            MLOG_ERROR("PwmExpanderPin: channel %d out of range (0-15)", pinNumber);
            return false;
        }
        _channel = pinNumber;
        _isSetup = true;
        // Start with channel off
        _driver->setPin(_channel, 0, false);
        return true;
    }

    int PwmExpanderPin::read()
    {
        // PCA9685 is output-only; return -1
        return -1;
    }

    bool PwmExpanderPin::write(uint8_t value)
    {
        return writePwm(value ? 4095u : 0u);
    }

    bool PwmExpanderPin::writePwm(uint16_t value)
    {
        if (!_isSetup || !_driver)
            return false;
        _driver->setPin(_channel, value, false);
        return true;
    }

    int PwmExpanderPin::getPinNumber() const
    {
        return _channel;
    }

    bool PwmExpanderPin::isConfigured() const
    {
        return _isSetup && _channel >= 0;
    }

    String PwmExpanderPin::toString() const
    {
        return _expanderId + ":" + String(_channel);
    }

} // namespace pins
