/**
 * @file Led.cpp
 * @brief Simple LED implementation using Device, ConfigMixin, and StateMixin
 */

#include "devices/Led.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{
    static bool _isPrevBlinkingOn = false;

    Led::Led(const String &id)
        : Device(id, "led"), _pin(nullptr)
    {
    }

    Led::~Led()
    {
        if (_pin != nullptr)
        {
            delete _pin;
            _pin = nullptr;
        }
    }

    void Led::setup()
    {
        Device::setup();

        // Set the device name
        setName(_config.name);

        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured (pin = -1)", toString().c_str());
            return;
        }

        // Create the appropriate pin type based on configuration
        switch (_config.pinType)
        {
        case LedPinType::GPIO:
            _pin = new pins::GpioPin();
            break;
        case LedPinType::I2C_PCF8574:
            _pin = new pins::I2cExpanderPin(pins::I2cExpanderType::PCF8574, _config.i2cAddress);
            break;
        case LedPinType::I2C_PCF8575:
            _pin = new pins::I2cExpanderPin(pins::I2cExpanderType::PCF8575, _config.i2cAddress);
            break;
        case LedPinType::I2C_MCP23017:
            _pin = new pins::I2cExpanderPin(pins::I2cExpanderType::MCP23017, _config.i2cAddress);
            break;
        default:
            _pin = new pins::GpioPin();
            break;
        }

        if (!_pin->setup(_config.pin, pins::PinMode::Output))
        {
            MLOG_ERROR("%s: Failed to setup pin %d", toString().c_str(), _config.pin);
            delete _pin;
            _pin = nullptr;
            return;
        }
        MLOG_INFO("%s: Setup on %s", toString().c_str(), _pin->toString().c_str());

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
        if (!_pin->isConfigured())
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        // Skip if already OK
        if (_state.mode == (value ? "ON" : "OFF"))
        {
            return true;
        }

        // Update state
        _state.mode = value ? "ON" : "OFF";

        _pin->write(value ? HIGH : LOW);
        MLOG_INFO("%s: Set to %s", toString().c_str(), value ? "ON" : "OFF");

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    bool Led::blink(unsigned long onTime, unsigned long offTime, unsigned long delay)
    {
        if (!_pin->isConfigured())
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        // Skip if already OK
        if (_state.mode == "BLINKING" && _state.blinkOnTime == onTime && _state.blinkOffTime == offTime && _state.blinkDelay == delay)
        {
            return true;
        }

        // Update state
        _state.mode = "BLINKING";
        _state.blinkOnTime = onTime;
        _state.blinkOffTime = offTime;
        _state.blinkDelay = delay;

        // Pin set by loop()
        MLOG_INFO("%s: Blinking with delay=%lums, on=%lums, off=%lums (total cycle: %lums)",
                  toString().c_str(), delay, onTime, offTime, delay + onTime + offTime);

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    void Led::loop()
    {
        Device::loop();

        // -1: UnSet, 0: OFF: 1: ON
        static int _isPrevBlinkingOn = -1;

        if (!_pin->isConfigured() || _state.mode != "BLINKING")
        {
            _isPrevBlinkingOn = -1;
            return;
        }

        // Calculate total blink cycle time (including delay)
        unsigned long cycle = _state.blinkDelay + _state.blinkOnTime + _state.blinkOffTime;

        // Use modulo to find value in cycle (0 to cycle-1)
        unsigned long value = millis() % cycle;

        // Determine LED state based on value in cycle:
        // 0 to delay-1: OFF (delay period)
        // delay to delay+onTime-1: ON (on period)
        // delay+onTime to cycle-1: OFF (off period)
        bool shouldBeOn = (value >= _state.blinkDelay && value < _state.blinkDelay + _state.blinkOnTime);

        // Only update GPIO if state changed (check against current mode state)
        if ((shouldBeOn && _isPrevBlinkingOn != 1) || (!shouldBeOn && _isPrevBlinkingOn != 0))
        {
            _isPrevBlinkingOn = shouldBeOn ? 1 : 0;
            _pin->write(shouldBeOn ? HIGH : LOW);
        }
    }

    void Led::addStateToJson(JsonDocument &doc)
    {
        doc["mode"] = _state.mode;
        doc["blinkOnTime"] = _state.blinkOnTime;
        doc["blinkOffTime"] = _state.blinkOffTime;
        doc["blinkDelay"] = _state.blinkDelay;
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
            unsigned long delay = 0;

            if (args)
            {
                if ((*args)["onTime"].is<unsigned long>())
                    onTime = (*args)["onTime"].as<unsigned long>();
                if ((*args)["offTime"].is<unsigned long>())
                    offTime = (*args)["offTime"].as<unsigned long>();
                if ((*args)["delay"].is<unsigned long>())
                    delay = (*args)["delay"].as<unsigned long>();
            }

            return blink(onTime, offTime, delay);
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
        if (config["pinType"].is<String>())
        {
            String pinTypeStr = config["pinType"].as<String>();
            if (pinTypeStr == "GPIO" || pinTypeStr == "gpio")
            {
                _config.pinType = LedPinType::GPIO;
            }
            else if (pinTypeStr == "PCF8574" || pinTypeStr == "pcf8574")
            {
                _config.pinType = LedPinType::I2C_PCF8574;
            }
            else if (pinTypeStr == "PCF8575" || pinTypeStr == "pcf8575")
            {
                _config.pinType = LedPinType::I2C_PCF8575;
            }
            else if (pinTypeStr == "MCP23017" || pinTypeStr == "mcp23017")
            {
                _config.pinType = LedPinType::I2C_MCP23017;
            }
        }
        if (config["i2cAddress"].is<int>())
        {
            _config.i2cAddress = config["i2cAddress"].as<uint8_t>();
        }
    }

    void Led::configToJson(JsonDocument &doc)
    {
        doc["pin"] = _config.pin;
        doc["name"] = _config.name;
        doc["initialState"] = _config.initialState;
        
        // Serialize pin type
        switch (_config.pinType)
        {
        case LedPinType::GPIO:
            doc["pinType"] = "GPIO";
            break;
        case LedPinType::I2C_PCF8574:
            doc["pinType"] = "PCF8574";
            doc["i2cAddress"] = _config.i2cAddress;
            break;
        case LedPinType::I2C_PCF8575:
            doc["pinType"] = "PCF8575";
            doc["i2cAddress"] = _config.i2cAddress;
            break;
        case LedPinType::I2C_MCP23017:
            doc["pinType"] = "MCP23017";
            doc["i2cAddress"] = _config.i2cAddress;
            break;
        }
    }

} // namespace devices
