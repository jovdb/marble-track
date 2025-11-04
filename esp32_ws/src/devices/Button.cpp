#include "Logging.h"
/**
 * @file Button.cpp
 * @brief Implementation of Button input class
 *
 * This file contains the implementation of the Button class methods
 * for handling button input with debouncing in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Button.h"

/**
 * @brief Constructor for Button class
 *
 * Initializes the Button object with just the ID parameter.
 * @param id Unique identifier string for the button
 */
Button::Button(const String &id, NotifyClients callback)
    : Device(id, "button", callback)
{
    _pin = -1;
    _pinMode = PinModeOption::Floating;
    _debounceMs = 50;
    _buttonType = ButtonType::NormalOpen;
}

/**
 * @brief Setup function to initialize the button
 * Must be called in setup() before using the button
 */
void Button::setup()
{
    if (_pin < 0)
    {
        MLOG_WARN("Button [%s]: Pin not configured, skipping setup", _id.c_str());
        return;
    }

    // Configure pin mode based on configuration
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

    // Initialize state variables
    _rawState = readRawState();
    _currentState = _rawState;
}

/**
 * @brief Loop function for continuous button operations
 *
 * Handles debouncing logic and state change detection.
 * Should be called repeatedly in the main loop.
 */
void Button::loop()
{
    if (_pin < 0)
    {
        return;
    }

    // If we have a virtual press active, don't process physical button state
    if (_virtualPress)
    {
        return;
    }

    // Read current raw state
    bool newRawState = readRawState();

    // Check if raw state has changed
    if (newRawState != _rawState)
    {
        // Reset debounce timer and update raw state
        _lastDebounceTime = millis();
        _rawState = newRawState;
    }

    // Check if enough time has passed for debouncing
    if ((millis() - _lastDebounceTime) > _debounceMs)
    {
        // Update debounced state if it has changed
        if (_rawState != _currentState)
        {
            bool previousState = _currentState;
            _currentState = _rawState;

            // Set edge detection flags
            if (_currentState && !previousState)
            {
                // Button was pressed
                _pressedFlag = true;
                _pressStartTime = millis();
                MLOG_INFO("Button [%s]: PRESSED", _id.c_str());
                notifyStateChange(); // Notify state change
            }
            else if (!_currentState && previousState)
            {
                // Button was released
                _releasedFlag = true;
                MLOG_INFO("Button [%s]: RELEASED", _id.c_str());
                notifyStateChange(); // Notify state change
            }
        }
    }
}

/**
 * @brief Check if button was pressed (edge detection)
 * @return true once when button is pressed, false otherwise
 * @note This function clears the pressed flag, so it only returns true once per press
 */
bool Button::onPressed()
{
    if (_pressedFlag)
    {
        // TODO: I assume this can be only checked once
        _pressedFlag = false;
        return true;
    }
    return false;
}

/**
 * @brief Check if button was released (edge detection)
 * @return true once when button is released, false otherwise
 * @note This function clears the released flag, so it only returns true once per release
 */
bool Button::onReleased()
{
    if (_releasedFlag)
    {
        _releasedFlag = false;
        return true;
    }
    return false;
}

/**
 * @brief Get how long the button has been pressed
 * @return Time in milliseconds since button was pressed, 0 if not currently pressed
 */
unsigned long Button::getPressedTime() const
{
    if (_currentState)
    {
        return millis() - _pressStartTime;
    }
    return 0;
}

/**
 * @brief Get the pins used by this button
 * @return Vector of pin numbers, empty if not configured
 */
std::vector<int> Button::getPins() const
{
    if (_pin < 0)
    {
        return {};
    }
    return {_pin};
}

/**
 * @brief Dynamic control function for button operations
 * @param action The action to perform ("pressed", "released")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Button::control(const String &action, JsonObject *args)
{
    if (action == "pressed")
    {
        // Simulate button press: set state to pressed and trigger notifications
        if (!_currentState)
        {
            _currentState = true;
            _pressedFlag = true;
            _pressStartTime = millis();
            _virtualPress = true; // Mark as virtual press to prevent physical override
            MLOG_INFO("Button [%s]: Simulated PRESS", _id.c_str());
            notifyStateChange();
            return true;
        }
        else
        {
            MLOG_WARN("Button [%s]: Already pressed, ignoring simulated press", _id.c_str());
            return false;
        }
    }
    else if (action == "released")
    {
        // Simulate button release: set state to released and trigger notifications
        if (_currentState)
        {
            _currentState = false;
            _releasedFlag = true;
            _virtualPress = false; // Clear virtual press flag
            unsigned long pressDuration = millis() - _pressStartTime;
            MLOG_INFO("Button [%s]: Simulated RELEASE (held for %lu ms)", _id.c_str(), pressDuration);
            notifyStateChange();
            return true;
        }
        else
        {
            MLOG_WARN("Button [%s]: Already released, ignoring simulated release", _id.c_str());
            return false;
        }
    }
    else
    {
        MLOG_WARN("Button [%s]: Unknown action: %s", _id.c_str(), action.c_str());
        return false;
    }
}

/**
 * @brief Get current state of the button
 * @return String containing JSON representation of the current state
 */
String Button::getState()
{
    JsonDocument doc;

    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }

    // Add Button-specific fields
    doc["pressed"] = _currentState;
    doc["pressedTime"] = getPressedTime();

    String result;
    serializeJson(doc, result);
    return result;
}

String Button::getConfig() const
{
    DynamicJsonDocument config(256);
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;
    config["pinMode"] = pinModeToString(_pinMode);
    config["debounceMs"] = _debounceMs;
    config["buttonType"] = buttonTypeToString(_buttonType);

    String message;
    serializeJson(config, message);
    return message;
}

void Button::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Button [%s]: Null config provided", _id.c_str());
        return;
    }

    const JsonVariant nameVar = (*config)["name"];
    if (!nameVar.isNull())
    {
        _name = nameVar.as<String>();
    }

    const JsonVariant pinVar = (*config)["pin"];
    if (!pinVar.isNull())
    {
        _pin = pinVar.as<int>();
    }

    const JsonVariant pinModeVar = (*config)["pinMode"];
    if (!pinModeVar.isNull())
    {
        if (pinModeVar.is<String>())
        {
            _pinMode = pinModeFromString(pinModeVar.as<String>());
        }
        else if (pinModeVar.is<int>() || pinModeVar.is<long>())
        {
            const int value = pinModeVar.as<int>();
            switch (value)
            {
            case 1:
                _pinMode = PinModeOption::PullUp;
                break;
            case 2:
                _pinMode = PinModeOption::PullDown;
                break;
            case 0:
            default:
                _pinMode = PinModeOption::Floating;
                break;
            }
        }
    }
    const JsonVariant debounceVar = (*config)["debounceMs"];
    if (!debounceVar.isNull())
    {
        const long debounceValue = debounceVar.as<long>();
        if (debounceValue < 0)
        {
            const long clampedValue = 0;
            _debounceMs = static_cast<unsigned long>(clampedValue);
        }
        else
        {
            _debounceMs = static_cast<unsigned long>(debounceValue);
        }
    }

    const JsonVariant buttonTypeVar = (*config)["buttonType"];
    if (!buttonTypeVar.isNull())
    {
        if (buttonTypeVar.is<String>())
        {
            _buttonType = buttonTypeFromString(buttonTypeVar.as<String>());
        }
        else if (buttonTypeVar.is<int>() || buttonTypeVar.is<long>())
        {
            const int value = buttonTypeVar.as<int>();
            _buttonType = (value == 1) ? ButtonType::NormalClosed : ButtonType::NormalOpen;
        }
    }

    // Apply pin mode after updating config
    setup();
}

/**
 * @brief Read the raw pin state accounting for pull-up/pull-down configuration
 * @return true if button is physically pressed, false otherwise
 */
bool Button::readRawState()
{
    if (_pin < 0)
    {
        return false;
    }

    bool pinState = digitalRead(_pin);

    // Determine pressed state depending on pin mode
    bool pressed;
    pressed = !pinState;

    return pressed;
}

Button::ButtonType Button::buttonTypeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("NormalClosed") || value.equalsIgnoreCase("NC"))
    {
        return ButtonType::NormalClosed;
    }
    return ButtonType::NormalOpen;
}

Button::PinModeOption Button::pinModeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("pullup"))
    {
        return PinModeOption::PullUp;
    }
    if (value.equalsIgnoreCase("pulldown"))
    {
        return PinModeOption::PullDown;
    }
    return PinModeOption::Floating;
}

String Button::pinModeToString(PinModeOption mode) const
{
    switch (mode)
    {
    case PinModeOption::PullUp:
        return "pullup";
    case PinModeOption::PullDown:
        return "pulldown";
    case PinModeOption::Floating:
    default:
        return "floating";
    }
}

String Button::buttonTypeToString(ButtonType type) const
{
    return (type == ButtonType::NormalClosed) ? "NormalClosed" : "NormalOpen";
}
