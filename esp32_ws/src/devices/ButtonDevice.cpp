#include "devices/ButtonDevice.h"
#include "Logging.h"

ButtonDevice::ButtonDevice(const String &id, NotifyClients callback) : ControllableTaskDevice(id, "button", callback)
{
}

void ButtonDevice::getConfigFromJson(const JsonDocument &config)
{
    if (config["name"].is<String>())
        _name = config["name"].as<String>();
    if (config["pin"].is<int>())
        _pin = config["pin"].as<int>();
    
    _debounceTimeInMs = config["debounceTimeInMs"] | 50UL;

    if (config["pinMode"].is<String>())
    {
        _pinMode = pinModeFromString(config["pinMode"].as<String>());
    }

    if (config["buttonType"].is<String>())
    {
        _buttonType = buttonTypeFromString(config["buttonType"].as<String>());
    }

    // Initialize state
    _simulatedIsPressed = (_buttonType == ButtonType::NormalClosed);
    _isPressed = _simulatedIsPressed.load();
    _lastRawValue = contactStateToPinState(_isPressed);

    if (_pin >= 0)
    {
        switch (_pinMode)
        {
        case PinModeOption::PullUp:
            pinMode(_pin, INPUT_PULLUP);
            break;
        case PinModeOption::PullDown:
            pinMode(_pin, INPUT_PULLDOWN);
            break;
        case PinModeOption::Floating:
        default:
            pinMode(_pin, INPUT);
            break;
        }
    }
    else
    {
        MLOG_WARN("%s: No valid pin configured", toString().c_str());
    }
}

void ButtonDevice::addConfigToJson(JsonDocument &doc) const
{
    doc["name"] = _name;
    doc["pin"] = _pin;
    doc["debounceTimeInMs"] = _debounceTimeInMs;
    doc["pinMode"] = pinModeToString(_pinMode);
    doc["buttonType"] = buttonTypeToString(_buttonType);
}

void ButtonDevice::addStateToJson(JsonDocument &doc)
{
    doc["value"] = _lastRawValue.load();
    doc["isPressed"] = _isPressed.load();
}

bool ButtonDevice::control(const String &action, JsonObject *args)
{
    if (action == "press" || action == "release")
    {
        bool isPress = (action == "press");
        MLOG_INFO("%s: Simulated button %s", toString().c_str(), isPress ? "PRESS" : "RELEASE");
        
        _isSimulated = true;
        // Pressing: NO closes (true), NC opens (false)
        // Releasing: NO opens (false), NC closes (true)
        if (isPress) {
            _simulatedIsPressed = (_buttonType == ButtonType::NormalOpen);
        } else {
            _simulatedIsPressed = (_buttonType == ButtonType::NormalClosed);
        }
        return true;
    }
    return false;
}

std::vector<int> ButtonDevice::getPins() const
{
    if (_pin >= 0)
        return {_pin};
    return {};
}

void ButtonDevice::task()
{
    unsigned long lastDebounceTime = 0;
    bool lastIsButtonPressed = readIsButtonPressed();
    bool currentStableState = lastIsButtonPressed;
    _isPressed = currentStableState;

    while (true)
    {
        bool isButtonPressed = readIsButtonPressed();

        if (isButtonPressed != lastIsButtonPressed)
        {
            lastDebounceTime = millis();
            lastIsButtonPressed = isButtonPressed;
        }

        if ((millis() - lastDebounceTime) > _debounceTimeInMs)
        {
            if (isButtonPressed != currentStableState)
            {
                currentStableState = isButtonPressed;
                if (_isPressed.exchange(currentStableState) != currentStableState)
                {
                    MLOG_INFO("%s: Contact state changed to %s", toString().c_str(), currentStableState ? "CLOSED" : "OPEN");
                    notifyStateChange();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool ButtonDevice::readIsButtonPressed()
{
    if (_isSimulated.load())
    {
        bool isClosed = _simulatedIsPressed.load();
        _lastRawValue = contactStateToPinState(isClosed);
        return isClosed;
    }

    if (_pin < 0)
        return false;

    int pinState = digitalRead(_pin);
    _lastRawValue = pinState;

    // Contact is closed if:
    // PullUp: pin is LOW
    // PullDown/Floating: pin is HIGH
    return (_pinMode == PinModeOption::PullUp) ? (pinState == LOW) : (pinState == HIGH);
}

int ButtonDevice::contactStateToPinState(bool isClosed) const
{
    if (_pinMode == PinModeOption::PullUp)
    {
        return isClosed ? LOW : HIGH;
    }
    else
    {
        return isClosed ? HIGH : LOW;
    }
}

String ButtonDevice::pinModeToString(PinModeOption mode) const
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

ButtonDevice::PinModeOption ButtonDevice::pinModeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("PullUp"))
        return PinModeOption::PullUp;
    if (value.equalsIgnoreCase("PullDown"))
        return PinModeOption::PullDown;
    return PinModeOption::Floating;
}

String ButtonDevice::buttonTypeToString(ButtonType type) const
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

ButtonDevice::ButtonType ButtonDevice::buttonTypeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("NormalClosed"))
        return ButtonType::NormalClosed;
    return ButtonType::NormalOpen;
}

bool ButtonDevice::isPressed() const
{
    return _isPressed.load();
}

bool ButtonDevice::isReleased() const
{
    return !_isPressed.load();
}
