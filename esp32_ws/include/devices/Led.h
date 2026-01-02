/**
 * @file Led.h
 * @brief Simple LED device using only Device (no mixins)
 */

#ifndef COMPOSITION_LED_H
#define COMPOSITION_LED_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"

namespace devices
{

    /**
     * @struct LedConfig
     * @brief Configuration for LED device
     */
    struct LedConfig
    {
        int pin = -1;                    // GPIO pin number (-1 = not configured)
        String name = "Led";             // Device name
        String initialState = "OFF";     // Initial state: OFF, ON, or BLINKING
    };

    /**
     * @struct LedState
     * @brief State structure for LED device
     */
    struct LedState
    {
        String mode = "OFF"; // OFF, ON, or BLINKING
        unsigned long blinkOnTime = 500;
        unsigned long blinkOffTime = 500;
        unsigned long blinkDelay = 0; // Delay before starting blink cycle
    };

    /**
     * @class Led
     * @brief LED with configurable pin, state management, and control interface
     */
    class Led : public Device, public ConfigMixin<Led, LedConfig>, public StateMixin<Led, LedState>, public ControllableMixin<Led>, public SerializableMixin<Led>
    {
    public:
        explicit Led(const String &id);

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        bool set(bool value);
        bool blink(unsigned long onTime = 500, unsigned long offTime = 500, unsigned long delay = 0);

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation (ISerializable interface)
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

            };

} // namespace devices

#endif // COMPOSITION_LED_H


