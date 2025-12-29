/**
 * @file Button.h
 * @brief Button device using DeviceBase composition pattern with mixins
 */

#ifndef COMPOSITION_BUTTON_H
#define COMPOSITION_BUTTON_H

#include "devices/DeviceBase.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"

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
        int pin = -1;                                    // GPIO pin number (-1 = not configured)
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
    };

    /**
     * @class Button
     * @brief Button with configurable pin, state management, and control interface
     */
    class Button : public DeviceBase,
                   public ConfigMixin<Button, ButtonConfig>,
                   public StateMixin<Button, ButtonState>,
                   public ControllableMixin<Button>,
                   public SerializableMixin<Button>
    {
    public:
        explicit Button(const String &id);

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

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

        /**
         * @brief Check if the button state changed in the last loop
         * @return true if state changed, false otherwise
         */
        bool isStateChanged() const { return _isStateChanged; }

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

        // Can be used for edge detection and simulating events on change
        bool _isStateChanged = false;
    };

} // namespace devices

#endif // COMPOSITION_BUTTON_H
