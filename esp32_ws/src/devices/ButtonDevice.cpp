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

    // Initialize simulated state based on pin mode (released state)
    int defaultState = getDefaultRawValue();
    // For NC, the default state is "Pressed" (Closed contact)
    // For NO, the default state is "Released" (Open contact)
    _simulatedIsPressed = (_buttonType == ButtonType::NormalClosed);
    _lastRawValue = defaultState;

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
    if (action == "press")
    {
        MLOG_INFO("%s: Simulated button PRESS", toString().c_str());
        _isSimulated = true;
        // For NO, press means contact closed (true). For NC, press means contact open (false).
        _simulatedIsPressed = (_buttonType == ButtonType::NormalOpen);
        return true;
    }
    else if (action == "release")
    {
        MLOG_INFO("%s: Simulated button RELEASE", toString().c_str());
        _isSimulated = true;
        // For NO, release means contact open (false). For NC, release means contact closed (true).
        _simulatedIsPressed = (_buttonType == ButtonType::NormalClosed);
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
    // Snapshot variables
    unsigned long lastDebounceTime = 0;
    bool lastIsButtonPressed = false;
    /** Debounced */
    bool currentStableState = false;

    // Initial read
    if (_pin >= 0)
    {
        lastIsButtonPressed = readIsButtonPressed();
        currentStableState = lastIsButtonPressed;
        _isPressed = currentStableState;
    }

    while (true)
    {
        if (_pin < 0 && !_isSimulated.load())
        {
            // recheck every second
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        bool isButtonPressed;
        if (_isSimulated.load())
        {
            isButtonPressed = _simulatedIsPressed.load();
            // Update raw value for state reporting
            int defaultVal = getDefaultRawValue();
            if (_buttonType == ButtonType::NormalOpen)
            {
                _lastRawValue = isButtonPressed ? (defaultVal == 1 ? 0 : 1) : defaultVal;
            }
            else
            {
                _lastRawValue = isButtonPressed ? defaultVal : (defaultVal == 1 ? 0 : 1);
            }
        }
        else
        {
            isButtonPressed = readIsButtonPressed();
        }

        // If raw state changed, reset timer
        if (isButtonPressed != lastIsButtonPressed)
        {
            lastDebounceTime = millis();
            lastIsButtonPressed = isButtonPressed;
        }

        // Check debounce
        if ((millis() - lastDebounceTime) > _debounceTimeInMs)
        {
            if (isButtonPressed != currentStableState)
            {
                currentStableState = isButtonPressed;

                // Update shared variables
                bool prevPressed = _isPressed.load();
                _isPressed = currentStableState;

                if (_isPressed.load() != prevPressed)
                {
                    MLOG_INFO("%s: State changed to %s", toString().c_str(), _isPressed.load() ? "PRESSED" : "RELEASED");
                    notifyStateChange();
                }
            }
        }

        // Poll every 10ms for changes
        vTaskDelay(pdMS_TO_TICKS(10)); // Poll every 10ms
    }
}

bool ButtonDevice::readIsButtonPressed()
{
    if (_pin < 0)
        return false;

    int pinState = digitalRead(_pin);
    _lastRawValue = pinState;

    int defaultState = getDefaultRawValue();
    
    if (_buttonType == ButtonType::NormalOpen)
    {
        // For NO, it's pressed (closed) when it's NOT the default state
        return pinState != defaultState;
    }
    else
    {
        // For NC, it's pressed (closed) when it IS the default state
        return pinState == defaultState;
    }
}

int ButtonDevice::getDefaultRawValue() const
{
    // Default state = released / not pressed state:
    // |          | NO | NC |
    // |----------|----|----|
    // | PullUp   | 1  | 0  |
    // | PullDown | 0  | 1  |
    // | Floating | 0  | 1  |
    if (_buttonType == ButtonType::NormalOpen)
    {
        return (_pinMode == PinModeOption::PullUp) ? 1 : 0;
    }
    else
    {
        return (_pinMode == PinModeOption::PullUp) ? 0 : 1;
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
