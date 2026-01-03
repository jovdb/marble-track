/**
 * @file GpioPin.cpp
 * @brief Native GPIO pin implementation
 */

#include "pins/GpioPin.h"
#include "Logging.h"

namespace pins
{
GpioPin::GpioPin()
    : _pinNumber(-1), _isSetup(false), _mode(PinMode::Input)
    {
    }

    void GpioPin::setPinNumber(int pinNumber)
    {
        if (_isSetup)
        {
            MLOG_WARN("GpioPin: Cannot change pin number after setup (was %d, tried %d)", _pinNumber, pinNumber);
            return;
        }
        _pinNumber = pinNumber;
    }

    bool GpioPin::setup(PinMode mode)
    {
        if (_pinNumber < 0)
        {
            MLOG_WARN("GpioPin: Cannot setup unconfigured pin (pin = -1)");
            return false;
        }

        _mode = mode;

        // Convert our PinMode to Arduino pinMode
        uint8_t arduinoMode;
        switch (mode)
        {
        case PinMode::Input:
            arduinoMode = INPUT;
            break;
        case PinMode::InputPullUp:
            arduinoMode = INPUT_PULLUP;
            break;
        case PinMode::InputPullDown:
            arduinoMode = INPUT_PULLDOWN;
            break;
        case PinMode::Output:
            arduinoMode = OUTPUT;
            break;
        default:
            arduinoMode = INPUT;
            break;
        }

        pinMode(_pinNumber, arduinoMode);
        _isSetup = true;

        MLOG_INFO("GpioPin: Setup %s", toString().c_str());
        return true;
    }

    int GpioPin::read()
    {
        if (!_isSetup || _pinNumber < 0)
        {
            return -1;
        }
        return digitalRead(_pinNumber);
    }

    bool GpioPin::write(uint8_t value)
    {
        if (!_isSetup || _pinNumber < 0)
        {
            MLOG_WARN("GpioPin: Cannot write to unconfigured pin");
            return false;
        }

        if (_mode != PinMode::Output)
        {
            MLOG_WARN("GpioPin: Writing to non-output pin %d", _pinNumber);
        }

        digitalWrite(_pinNumber, value);
        return true;
    }

    int GpioPin::getPinNumber() const
    {
        return _pinNumber;
    }

    bool GpioPin::isConfigured() const
    {
        return _pinNumber >= 0 && _isSetup;
    }

    String GpioPin::toString() const
    {
        if (_pinNumber < 0)
        {
            return "GPIO:unconfigured";
        }

        String modeStr;
        switch (_mode)
        {
        case PinMode::Input:
            modeStr = "INPUT";
            break;
        case PinMode::InputPullUp:
            modeStr = "INPUT_PULLUP";
            break;
        case PinMode::InputPullDown:
            modeStr = "INPUT_PULLDOWN";
            break;
        case PinMode::Output:
            modeStr = "OUTPUT";
            break;
        }

        return "GPIO:" + String(_pinNumber) + "(" + modeStr + ")";
    }

} // namespace pins
