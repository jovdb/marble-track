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
    if (config["debounceTimeInMs"].is<unsigned long>())
        _debounceTimeInMs = config["debounceTimeInMs"].as<unsigned long>();

    if (config["pinMode"].is<String>())
    {
        _pinMode = pinModeFromString(config["pinMode"].as<String>());
    }

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
}

void ButtonDevice::addConfigToJson(JsonDocument &doc) const
{
    doc["name"] = _name;
    doc["pin"] = _pin;
    doc["debounceTimeInMs"] = _debounceTimeInMs;
    doc["pinMode"] = pinModeToString(_pinMode);
}

void ButtonDevice::addStateToJson(JsonDocument &doc)
{
    doc["isPressed"] = _isPressed;
}

bool ButtonDevice::control(const String &action, JsonObject *args)
{
    if (action == "press")
    {
        _isSimulated = true;
        _simulatedState = true;
        MLOG_INFO("%s: Simulated  button PRESS", toString().c_str());
        return true;
    }
    else if (action == "release")
    {
        _isSimulated = false;
        MLOG_INFO("%s: Simulated button RELEASE", toString().c_str());
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
        if (_pin < 0)
        {
            // recheck every second
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        bool isButtonPressed = _isSimulated ? _simulatedState : readIsButtonPressed();

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
                bool prevPressed = _isPressed;
                _isPressed = currentStableState;

                if (_isPressed != prevPressed)
                {
                    notifyState(true);
                    MLOG_INFO("[%s]: State changed to %s", toString().c_str(), _isPressed ? "PRESSED" : "RELEASED");
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

    bool pinState = digitalRead(_pin);

    // HIGH = Pressed for PullDown, LOW = Pressed for PullUp or Floating
    return _pinMode == PinModeOption::PullDown ? pinState : !pinState;
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

bool ButtonDevice::isPressed() const
{
    return _isPressed;
}

bool ButtonDevice::isReleased() const
{
    return !_isPressed;
}
