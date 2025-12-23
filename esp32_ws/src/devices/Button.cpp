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
    // Call base setup first (though Button has no children, this maintains consistency)
    Device::setup();

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
    _lastRawValue = getDefaultRawValue();
    // For NC, the default state is "Pressed" (Closed contact)
    // For NO, the default state is "Released" (Open contact)
    bool isPressed = readRawState();
    _currentState = isPressed;
    _newStableState = _currentState;
}

void Button::task()
{
    unsigned long lastDebounceTime = 0;
    bool lastIsPressed = readRawState();
    bool currentStableState = lastIsPressed;

    while (true)
    {
        if (_pin < 0 && !_virtualPress)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Read raw state and determine if pressed
        bool isPressed;
        if (_virtualPress)
        {
            isPressed = _currentState;
            // Update raw value for state reporting
            int defaultVal = getDefaultRawValue();
            if (_buttonType == ButtonType::NormalOpen)
            {
                _lastRawValue = isPressed ? (defaultVal == 1 ? 0 : 1) : defaultVal;
            }
            else
            {
                _lastRawValue = isPressed ? defaultVal : (defaultVal == 1 ? 0 : 1);
            }
        }
        else
        {
            isPressed = readRawState();
        }

        // If state changed, reset timer
        if (isPressed != lastIsPressed)
        {
            lastDebounceTime = millis();
            lastIsPressed = isPressed;
        }

        // Check debounce
        if ((millis() - lastDebounceTime) > _debounceMs)
        {
            if (isPressed != currentStableState)
            {
                currentStableState = isPressed;

                // Update shared variables for main loop
                _newStableState = currentStableState;
                _hasNewState = true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Poll every 10ms
    }
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

    // Check if task has reported a new state
    if (_hasNewState)
    {
        bool newState = _newStableState;
        _hasNewState = false; // Clear flag

        if (newState != _currentState)
        {
            bool previousState = _currentState;
            _currentState = newState;

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
    bool isPressAction = (action == "press" || action == "pressed");
    bool isReleaseAction = (action == "release" || action == "released");

    if (isPressAction || isReleaseAction)
    {
        bool targetContactState;
        if (isPressAction)
        {
            // Pressing: NO closes (true), NC opens (false)
            targetContactState = (_buttonType == ButtonType::NormalOpen);
        }
        else
        {
            // Releasing: NO opens (false), NC closes (true)
            targetContactState = (_buttonType == ButtonType::NormalClosed);
        }

        if (_currentState != targetContactState)
        {
            bool previousState = _currentState;
            _currentState = targetContactState;
            _virtualPress = true;

            if (isPressAction)
            {
                _pressedFlag = true;
                _pressStartTime = millis();
                MLOG_INFO("Button [%s]: Simulated PRESS", _id.c_str());
            }
            else
            {
                _releasedFlag = true;
                unsigned long pressDuration = millis() - _pressStartTime;
                MLOG_INFO("Button [%s]: Simulated RELEASE (held for %lu ms)", _id.c_str(), pressDuration);
            }

            notifyStateChange();
            return true;
        }
        return false;
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
    doc["isPressed"] = _currentState;
    doc["value"] = _lastRawValue;
    doc["pressedTime"] = getPressedTime();

    String result;
    serializeJson(doc, result);
    return result;
}

String Button::getConfig() const
{
    JsonDocument config;
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;
    config["pinMode"] = pinModeToString(_pinMode);
    config["debounceTimeInMs"] = _debounceMs;
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
    
    _debounceMs = (*config)["debounceTimeInMs"] | 50UL;

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
 *
 * 
 *  Default state / released:
 * |          | NO | NC |
 * |----------|----|----|
 * | PullUp   | 1  | 0  |
 * | PullDown | 0  | 1  |
 * | Floating | 0  | 1  |
 *

 * @return true if button is physically pressed, false otherwise
 */
bool Button::readRawState()
{
    if (_pin < 0)
    {
        return false;
    }

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

int Button::getDefaultRawValue() const
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
