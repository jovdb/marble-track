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
 * Initializes the Button object with pin, id, name, and configuration parameters.
 * @param pin GPIO pin number for the button
 * @param id Unique identifier string for the button
 * @param name Human-readable name string for the button
 * @param pullUp true for internal pull-up (button connects to ground), false for pull-down
 * @param debounceMs Debounce time in milliseconds
 */
Button::Button(const String &id, const String &name)
    : Device(id, "button")
{
    _name = name;
    _pin = -1;
    _pullUp = true;
    _debounceMs = 50;
    _buttonType = ButtonType::NormalOpen;
}

/**
 * @brief Setup function to initialize the button
 * Must be called in setup() before using the button
 */
void Button::setup()
{
    // Configure pin mode based on pull-up/pull-down setting
    if (_pullUp)
    {
        pinMode(_pin, INPUT_PULLUP);
    }
    else
    {
        pinMode(_pin, INPUT);
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
                unsigned long pressDuration = millis() - _pressStartTime;
                MLOG_INFO("Button [%s]: RELEASED (held for %lu ms)", _id.c_str(), pressDuration);
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
bool Button::wasPressed()
{
    if (_pressedFlag)
    {
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
bool Button::wasReleased()
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
    doc["pullUp"] = _pullUp;
    doc["debounceMs"] = _debounceMs;
    doc["buttonType"] = (_buttonType == ButtonType::NormalOpen) ? "NormalOpen" : "NormalClosed";

    String result;
    serializeJson(doc, result);
    return result;
}

/**
 * @brief Read the raw pin state accounting for pull-up/pull-down configuration
 * @return true if button is physically pressed, false otherwise
 */
bool Button::readRawState()
{
    bool pinState = digitalRead(_pin);

    // Invert logic for pull-up configuration (pin is LOW when button is pressed)
    bool pressed = _pullUp ? !pinState : pinState;
    // If NormalClosed, invert the pressed logic
    if (_buttonType == ButtonType::NormalClosed)
    {
        pressed = !pressed;
    }
    return pressed;
}
