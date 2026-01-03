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
#include "pins/IPin.h"
#include "pins/GpioPin.h"
#include "pins/I2cExpanderPin.h"

namespace devices
{

    /**
     * @struct LedConfig
     * @brief Configuration for LED device
     */
    /**
     * @brief Pin source type for LED
     */
    enum class LedPinType
    {
        GPIO,       // Native ESP32 GPIO pin
        I2C_PCF8574,  // PCF8574 I2C expander
        I2C_PCF8575,  // PCF8575 I2C expander
        I2C_MCP23017  // MCP23017 I2C expander
    };

    /**
     * @struct LedConfig
     * @brief Configuration for LED device
     */
    struct LedConfig
    {
        int pin = -1;                    // GPIO or expander pin number (-1 = not configured)
        String name = "Led";             // Device name
        String initialState = "OFF";     // Initial state: OFF, ON, or BLINKING
        
        // I2C expander settings (only used when pinType != GPIO)
        LedPinType pinType = LedPinType::GPIO;  // Pin source type
        uint8_t i2cAddress = 0x20;              // I2C address for expander (default 0x20)
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

        ~Led() override;

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

    private:
        pins::IPin* _pin;  // Pin abstraction for LED output
    };

} // namespace devices

#endif // COMPOSITION_LED_H


