/**
 * @file Button.h
 * @brief Button device using Device composition pattern with mixins
 */

#ifndef COMPOSITION_BUTTON_H
#define COMPOSITION_BUTTON_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "pins/IPin.h"
#include "pins/Pins.h"

namespace devices
{

    /**
     * @brief Button type enumeration
     */
    enum class ButtonType
    {
        NormalOpen,
        NormalClosed
    };

    /**
     * @brief Pin mode enumeration
     */
    enum class PinModeOption
    {
        Floating,
        PullUp,
        PullDown
    };

    /**
     * @struct ButtonConfig
     * @brief Configuration for Button device
     */
    struct ButtonConfig
    {
        PinConfig pinConfig;                             // Pin configuration (type, address, pin number)
        String name = "Button";                          // Device name
        unsigned long debounceTimeInMs = 50;             // Debounce time in milliseconds
        PinModeOption pinMode = PinModeOption::Floating; // Pin mode
        ButtonType buttonType = ButtonType::NormalOpen;  // Button type
    };

    /**
     * @struct ButtonState
     * @brief State structure for Button device
     */
    struct ButtonState
    {
        /** Indicates it is currently not in its default state */
        bool isPressed = false;

        /** pin input state: 0=LOW, 1=HIGH */
        int input = 0;

        /** Indicates if the button state changed in the last loop */
        bool isPressedChanged = false;
    };

    /**
     * @class Button
     * @brief Button with configurable pin, state management, and control interface
     */
    class Button : public Device,
                   public ConfigMixin<Button, ButtonConfig>,
                   public StateMixin<Button, ButtonState>,
                   public ControllableMixin<Button>,
                   public SerializableMixin<Button>
    {
    public:
        explicit Button(const String &id);

        ~Button() override;

        void setup() override;
        void teardown() override;
        void loop() override;
        std::vector<String> getPins() const override;

        /**
         * @brief Get the current pressed state of the button
         * @return true if pressed, false otherwise
         */
        bool isPressed() const;

        /**
         * @brief Get the current released state of the button
         * @return true if released, false otherwise
         */
        bool isReleased() const;

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation (ISerializable interface)
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool readIsButtonPressed();
        String pinModeToString(PinModeOption mode) const;
        PinModeOption pinModeFromString(const String &value) const;
        String buttonTypeToString(ButtonType type) const;
        ButtonType buttonTypeFromString(const String &value) const;
        int contactStateToPinState(bool isClosed) const;

        // Debounce state
        unsigned long _lastDebounceTime = 0;
        bool _lastIsButtonPressed = false;

        // Simulation support
        bool _isSimulated = false;
        bool _simulatedIsPressed = false;

        // Pin abstraction for button input
        pins::IPin* _pin;
    };

} // namespace devices

#endif // COMPOSITION_BUTTON_H
