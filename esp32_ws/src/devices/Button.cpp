/**
 * @file Button.cpp
 * @brief Button implementation using Device composition pattern with mixins
 */

#include "devices/Button.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    Button::Button(const String &id)
        : Device(id, "button"), _pin(nullptr)
    {
    }

    Button::~Button()
    {
        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }
    }

    void Button::setup()
    {
        Device::setup();

        // Set the device name
        setName(_config.name);

        // Clean up any existing pin
        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }

        if (_config.pinConfig.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return;
        }

        // Create the pin using the factory
        _pin = PinFactory::createPin(_config.pinConfig);
        if (!_pin)
        {
            MLOG_ERROR("%s: Failed to create pin for expander '%s'", toString().c_str(), _config.pinConfig.expanderId.c_str());
            return;
        }

        // Determine pin mode for setup
        pins::PinMode pinSetupMode;
        switch (_config.pinMode)
        {
        case PinModeOption::PullUp:
            pinSetupMode = pins::PinMode::InputPullUp;
            break;
        case PinModeOption::PullDown:
            pinSetupMode = pins::PinMode::InputPullDown;
            break;
        case PinModeOption::Floating:
        default:
            pinSetupMode = pins::PinMode::Input;
            break;
        }

        if (!_pin->setup(_config.pinConfig.pin, pinSetupMode))
        {
            MLOG_ERROR("%s: Failed to setup pin %d", toString().c_str(), _config.pinConfig.pin);
            //  delete _pin;
            //    _pin = nullptr;
            return;
        }

        // Initialize state
        _simulatedIsPressed = (_config.buttonType == ButtonType::NormalClosed);
        _state.isPressed = readIsButtonPressed();
        _state.input = contactStateToPinState(_state.isPressed);
        _lastIsButtonPressed = _state.isPressed;

        MLOG_INFO("%s: Setup on pin %s with pinMode %s", toString().c_str(), _pin->toString().c_str(), pinModeToString(_config.pinMode).c_str());
    }

    void Button::teardown()
    {
        Device::teardown();

        if (_config.pinConfig.expanderId.isEmpty() && _config.pinConfig.pin >= 0)
        {
            pinMode(_config.pinConfig.pin, INPUT);
        }

        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }

        _lastDebounceTime = 0;
        _lastIsButtonPressed = false;
        _isSimulated = false;
        _simulatedIsPressed = false;
        _state.isPressed = false;
        _state.isPressedChanged = false;
        _state.input = 0;
    }

    void Button::loop()
    {
        Device::loop();

        if (_pin == nullptr || !_pin->isConfigured())
            return;

        // Reset state change flag for next loop
        _state.isPressedChanged = false;

        bool isButtonPressed = readIsButtonPressed();

        if (isButtonPressed != _lastIsButtonPressed)
        {
            _lastDebounceTime = millis();
            _lastIsButtonPressed = isButtonPressed;
        }

        if ((millis() - _lastDebounceTime) > _config.debounceTimeInMs)
        {
            if (isButtonPressed != _state.isPressed)
            {
                _state.isPressed = isButtonPressed;
                _state.isPressedChanged = true;
                MLOG_INFO("%s: New button data read: %s", toString().c_str(), _state.input ? "HIGH" : "LOW");
                notifyStateChanged();
            }
        }
    }

    std::vector<String> Button::getPins() const
    {
        if (_pin)
        {
            String pinStr = _pin->toString();
            if (!pinStr.isEmpty())
                return {pinStr};
        }
        return {};
    }

    bool Button::isPressed() const
    {
        return _state.isPressed;
    }

    bool Button::isReleased() const
    {
        return !_state.isPressed;
    }

    void Button::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["value"] = _state.input;
        doc["isPressed"] = _state.isPressed;
    }

    bool Button::control(const String &action, JsonObject *args)
    {
        if (action == "press" || action == "release")
        {
            bool isPress = (action == "press");
            // MLOG_INFO("%s: Simulated button %s", toString().c_str(), isPress ? "PRESS" : "RELEASE");

            _isSimulated = true;
            // Pressing: NO closes (true), NC opens (false)
            // Releasing: NO opens (false), NC closes (true)
            if (isPress)
            {
                _simulatedIsPressed = (_config.buttonType == ButtonType::NormalOpen);
            }
            else
            {
                _simulatedIsPressed = (_config.buttonType == ButtonType::NormalClosed);
            }
            return true;
        }
        return false;
    }

    void Button::jsonToConfig(const JsonDocument &config)
    {
        _config.pinConfig = PinFactory::jsonToConfig(config["pin"]);

        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (config["debounceTimeInMs"].is<unsigned long>())
            _config.debounceTimeInMs = config["debounceTimeInMs"].as<unsigned long>();
        if (config["pinMode"].is<String>())
            _config.pinMode = pinModeFromString(config["pinMode"].as<String>());
        if (config["buttonType"].is<String>())
            _config.buttonType = buttonTypeFromString(config["buttonType"].as<String>());
    }

    void Button::configToJson(JsonDocument &doc)
    {
        JsonDocument pinDoc;
        PinFactory::configToJson(_config.pinConfig, pinDoc);

        doc["pin"] = pinDoc.as<JsonVariant>();
        doc["name"] = _config.name;
        doc["debounceTimeInMs"] = _config.debounceTimeInMs;
        doc["pinMode"] = pinModeToString(_config.pinMode);
        doc["buttonType"] = buttonTypeToString(_config.buttonType);
    }

    bool Button::readIsButtonPressed()
    {
        if (_isSimulated)
        {
            // _simulatedIsPressed stores the contact-closed state
            bool contactClosed = _simulatedIsPressed;
            _state.input = contactStateToPinState(contactClosed);
            // NC inversion always applies in simulation (no Floating exception needed)
            return (_config.buttonType == ButtonType::NormalClosed) ? !contactClosed : contactClosed;
        }

        if (_pin == nullptr || !_pin->isConfigured())
            return false;

        int pinState = _pin->read();
        _state.input = pinState;

        // Determine if the switch contact is electrically closed.
        // PullUp:           LOW  → contact pulls pin to GND  → contact closed
        // PullDown/Floating: HIGH → contact pulls pin to VCC → contact closed
        bool contactClosed = (_config.pinMode == PinModeOption::PullUp) ? (pinState == LOW) : (pinState == HIGH);

        // For NormallyClosed buttons with a defined pull resistor, pressing opens the
        // contact (inverts the closed sense).  Floating mode has no defined reference
        // level so NC acts identically to NO — no inversion.
        if (_config.buttonType == ButtonType::NormalClosed && _config.pinMode != PinModeOption::Floating)
            return !contactClosed;

        return contactClosed;
    }

    int Button::contactStateToPinState(bool isClosed) const
    {
        if (_config.pinMode == PinModeOption::PullUp)
        {
            return isClosed ? LOW : HIGH;
        }
        else
        {
            return isClosed ? HIGH : LOW;
        }
    }

    String Button::pinModeToString(PinModeOption mode) const
    {
        switch (mode)
        {
        case PinModeOption::PullUp:
            return "PullUp";
        case PinModeOption::PullDown:
            return "PullDown";
        case PinModeOption::Floating:
        default:
            return "Floating";
        }
    }

    PinModeOption Button::pinModeFromString(const String &value) const
    {
        if (value.equalsIgnoreCase("PullUp"))
            return PinModeOption::PullUp;
        if (value.equalsIgnoreCase("PullDown"))
            return PinModeOption::PullDown;
        return PinModeOption::Floating;
    }

    String Button::buttonTypeToString(ButtonType type) const
    {
        switch (type)
        {
        case ButtonType::NormalClosed:
            return "NormalClosed";
        case ButtonType::NormalOpen:
        default:
            return "NormalOpen";
        }
    }

    ButtonType Button::buttonTypeFromString(const String &value) const
    {
        if (value.equalsIgnoreCase("NormalClosed"))
            return ButtonType::NormalClosed;
        return ButtonType::NormalOpen;
    }

} // namespace devices
