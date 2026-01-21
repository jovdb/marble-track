/**
 * @file I2cExpanderPin.cpp
 * @brief I2C GPIO expander pin implementation
 */

#include "pins/I2cExpanderPin.h"
#include "Logging.h"

namespace pins
{
    // Static member initialization - support up to 16 different expanders
    uint16_t I2cExpanderPin::_portStates[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint16_t I2cExpanderPin::_portDirections[16] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                                    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    I2cExpanderPin::I2cExpanderPin(I2cExpanderType type, uint8_t i2cAddress, TwoWire *wireInstance, const String &expanderId)
        : _expanderType(type), _i2cAddress(i2cAddress), _wire(wireInstance), _expanderId(expanderId), _pinNumber(-1), _isSetup(false), _mode(PinMode::Input)
    {
    }

    uint8_t I2cExpanderPin::getCacheIndex() const
    {
        // Use lower 4 bits of address as index (addresses typically 0x20-0x27 or 0x38-0x3F)
        return _i2cAddress & 0x0F;
    }

    int I2cExpanderPin::getMaxPinNumber() const
    {
        switch (_expanderType)
        {
        case I2cExpanderType::PCF8574:
            return 7; // 8-bit expander (pins 0-7)
        case I2cExpanderType::PCF8575:
        case I2cExpanderType::MCP23017:
            return 15; // 16-bit expanders (pins 0-15)
        default:
            return 7;
        }
    }

    String I2cExpanderPin::getExpanderTypeName() const
    {
        switch (_expanderType)
        {
        case I2cExpanderType::PCF8574:
            return "PCF8574";
        case I2cExpanderType::PCF8575:
            return "PCF8575";
        case I2cExpanderType::MCP23017:
            return "MCP23017";
        default:
            return "Unknown";
        }
    }

    bool I2cExpanderPin::isDevicePresent()
    {
        _wire->beginTransmission(_i2cAddress);
        return (_wire->endTransmission() == 0);
    }

    bool I2cExpanderPin::setup(int pinNumber, PinMode mode)
    {
        if (pinNumber < 0 || pinNumber > getMaxPinNumber())
        {
            MLOG_WARN("I2cExpander: Invalid pin number %d for %s (max %d)",
                      pinNumber, getExpanderTypeName().c_str(), getMaxPinNumber());
            return false;
        }

        _pinNumber = pinNumber;
        _mode = mode;

        // Check if the I2C device is present
        if (!isDevicePresent())
        {
            MLOG_ERROR("I2cExpander: Device not found at address 0x%02X", _i2cAddress);
            return false;
        }

        // Configure the pin direction
        uint8_t cacheIdx = getCacheIndex();
        uint16_t pinMask = (1 << _pinNumber);

        if (mode == PinMode::Output)
        {
            // Set pin as output (0 = output for MCP23017, for PCF we just track it)
            _portDirections[cacheIdx] &= ~pinMask;

            // For PCF8574/8575, outputs are controlled by writing to the port
            // For MCP23017, we need to configure the IODIR register
            if (_expanderType == I2cExpanderType::MCP23017)
            {
                if (!configureDirection())
                {
                    MLOG_ERROR("I2cExpander: Failed to configure direction for pin %d", _pinNumber);
                    return false;
                }
            }

            // Initialize output to LOW
            _portStates[cacheIdx] &= ~pinMask;
            writePort(_portStates[cacheIdx]);
        }
        else
        {
            // Set pin as input (1 = input)
            _portDirections[cacheIdx] |= pinMask;

            // For MCP23017, configure IODIR and optionally pull-ups
            if (_expanderType == I2cExpanderType::MCP23017)
            {
                if (!configureDirection())
                {
                    MLOG_ERROR("I2cExpander: Failed to configure direction for pin %d", _pinNumber);
                    return false;
                }

                // Configure pull-ups if requested
                if (mode == PinMode::InputPullUp)
                {
                    // Set GPPU register for pull-up
                    uint8_t gppuReg = (_pinNumber < 8) ? 0x0C : 0x0D; // GPPUA or GPPUB
                    uint8_t bitPos = _pinNumber % 8;

                    _wire->beginTransmission(_i2cAddress);
                    _wire->write(gppuReg);
                    _wire->endTransmission();

                    _wire->requestFrom(_i2cAddress, (uint8_t)1);
                    uint8_t gppu = _wire->read();
                    gppu |= (1 << bitPos);

                    _wire->beginTransmission(_i2cAddress);
                    _wire->write(gppuReg);
                    _wire->write(gppu);
                    _wire->endTransmission();
                }
            }
            else
            {
                // For PCF8574/8575, set pin HIGH to enable input (quasi-bidirectional)
                _portStates[cacheIdx] |= pinMask;
                writePort(_portStates[cacheIdx]);
            }
        }

        _isSetup = true;
        MLOG_INFO("I2cExpander: Setup %s", toString().c_str());
        return true;
    }

    bool I2cExpanderPin::configureDirection()
    {
        if (_expanderType != I2cExpanderType::MCP23017)
        {
            return true; // Only MCP23017 needs direction configuration
        }

        uint8_t cacheIdx = getCacheIndex();

        // MCP23017 has separate IODIR registers for port A (pins 0-7) and port B (pins 8-15)
        // IODIRA = 0x00, IODIRB = 0x01
        uint8_t iodirA = _portDirections[cacheIdx] & 0xFF;
        uint8_t iodirB = (_portDirections[cacheIdx] >> 8) & 0xFF;

        // Write IODIRA
        _wire->beginTransmission(_i2cAddress);
        _wire->write(0x00); // IODIRA register
        _wire->write(iodirA);
        if (_wire->endTransmission() != 0)
        {
            return false;
        }

        // Write IODIRB
        _wire->beginTransmission(_i2cAddress);
        _wire->write(0x01); // IODIRB register
        _wire->write(iodirB);
        return (_wire->endTransmission() == 0);
    }

    uint16_t I2cExpanderPin::readPort()
    {
        uint16_t portValue = 0;

        switch (_expanderType)
        {
        case I2cExpanderType::PCF8574:
            _wire->requestFrom(_i2cAddress, (uint8_t)1);
            if (_wire->available())
            {
                portValue = _wire->read();
            }
            break;

        case I2cExpanderType::PCF8575:
            _wire->requestFrom(_i2cAddress, (uint8_t)2);
            if (_wire->available() >= 2)
            {
                portValue = _wire->read();
                portValue |= (_wire->read() << 8);
            }
            break;

        case I2cExpanderType::MCP23017:
            // Read GPIO registers (GPIOA = 0x12, GPIOB = 0x13)
            _wire->beginTransmission(_i2cAddress);
            _wire->write(0x12); // GPIOA register
            _wire->endTransmission();
            _wire->requestFrom(_i2cAddress, (uint8_t)2);
            if (_wire->available() >= 2)
            {
                portValue = _wire->read();         // GPIOA
                portValue |= (_wire->read() << 8); // GPIOB
            }
            break;
        }

        return portValue;
    }

    bool I2cExpanderPin::writePort(uint16_t state)
    {
        uint8_t result = 0;

        switch (_expanderType)
        {
        case I2cExpanderType::PCF8574:
            _wire->beginTransmission(_i2cAddress);
            _wire->write((uint8_t)(state & 0xFF));
            result = _wire->endTransmission();
            break;

        case I2cExpanderType::PCF8575:
            _wire->beginTransmission(_i2cAddress);
            _wire->write((uint8_t)(state & 0xFF));
            _wire->write((uint8_t)((state >> 8) & 0xFF));
            result = _wire->endTransmission();
            break;

        case I2cExpanderType::MCP23017:
            // Write to OLAT registers (OLATA = 0x14, OLATB = 0x15)
            _wire->beginTransmission(_i2cAddress);
            _wire->write(0x14); // OLATA register
            _wire->write((uint8_t)(state & 0xFF));
            _wire->write((uint8_t)((state >> 8) & 0xFF));
            result = _wire->endTransmission();
            break;
        }

        return (result == 0);
    }

    int I2cExpanderPin::read()
    {
        if (!_isSetup || _pinNumber < 0)
        {
            return -1;
        }

        uint16_t portValue = readPort();
        return (portValue & (1 << _pinNumber)) ? HIGH : LOW;
    }

    bool I2cExpanderPin::write(uint8_t value)
    {
        if (!_isSetup || _pinNumber < 0)
        {
            MLOG_WARN("I2cExpander: Cannot write to unconfigured pin");
            return false;
        }

        if (_mode != PinMode::Output)
        {
            MLOG_WARN("I2cExpander: Writing to non-output pin %d", _pinNumber);
        }

        uint8_t cacheIdx = getCacheIndex();
        uint16_t pinMask = (1 << _pinNumber);

        // Update cached state
        if (value)
        {
            _portStates[cacheIdx] |= pinMask;
        }
        else
        {
            _portStates[cacheIdx] &= ~pinMask;
        }

        // Write to expander
        return writePort(_portStates[cacheIdx]);
    }

    int I2cExpanderPin::getPinNumber() const
    {
        return _pinNumber;
    }

    bool I2cExpanderPin::isConfigured() const
    {
        return _pinNumber >= 0 && _isSetup;
    }

    String I2cExpanderPin::toString() const
    {
        if (_pinNumber < 0)
        {
            return "";
        }

        MLOG_INFO("I2cExpander: toString called, expanderId='%s'", _expanderId.c_str());
        if (!_expanderId.isEmpty())
        {
            return _expanderId + ":" + String(_pinNumber);
        }

        /*
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
        */

        char addrStr[8];
        snprintf(addrStr, sizeof(addrStr), "0x%02X", _i2cAddress);

        return "I²C:" + String(addrStr) + ":" + String(_pinNumber);
    }

} // namespace pins
