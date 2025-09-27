/**
 * @file Button.h
 * @brief Button input class for marble track system
 *
 * This class provides button input functionality with debouncing, state tracking,
 * and event detection for managing button interactions in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Device.h"

/**
 * @class Button
 * @brief Button input control class with debouncing
 *
 * Provides button input functionality with automatic debouncing, state tracking,
 * and event detection. Supports both pull-up and pull-down configurations.
 */
class Button : public Device
{
public:
    enum class ButtonType
    {
        NormalOpen,
        NormalClosed
    };

    Button(const String &id, const String &name);

    /**
     * @brief Setup function to initialize the button
     * Must be called in setup() before using the button
     */
    void setup();

    void loop() override; // Handles debouncing and state tracking

    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override { return {_pin}; }

    // Button-specific operations
    bool isPressed() const { return _currentState; }
    bool wasPressed();                    // Returns true once when button is pressed (edge detection)
    bool wasReleased();                   // Returns true once when button is released (edge detection)
    unsigned long getPressedTime() const; // How long button has been pressed (ms)

private:
    int _pin;                                        ///< GPIO pin number for the button
    bool _pullUp;                                    ///< Pull-up configuration (true = internal pull-up)
    unsigned long _debounceMs;                       ///< Debounce time in milliseconds
    ButtonType _buttonType = ButtonType::NormalOpen; ///< Button type (NO/NC)

    // State tracking
    bool _currentState = false; ///< Current debounced state
    bool _rawState = false;     ///< Raw pin reading

    // Timing
    unsigned long _lastDebounceTime = 0; ///< Last time the pin state changed
    unsigned long _pressStartTime = 0;   ///< When the current press started

    // Edge detection flags
    bool _pressedFlag = false;  ///< Set when button is pressed (cleared by wasPressed())
    bool _releasedFlag = false; ///< Set when button is released (cleared by wasReleased())
    bool _virtualPress = false; ///< Flag to indicate virtual button press is active

    /**
     * @brief Read the raw pin state accounting for pull-up/pull-down configuration
     * @return true if button is physically pressed, false otherwise
     */
    bool readRawState();
    ButtonType buttonTypeFromString(const String &value) const;
    String buttonTypeToString(ButtonType type) const;
};

#endif // BUTTON_H
