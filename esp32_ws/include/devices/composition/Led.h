/**
 * @file Led.h
 * @brief Simple LED device using only DeviceBase (no mixins)
 */

#ifndef COMPOSITION_LED_H
#define COMPOSITION_LED_H

#include "devices/composition/DeviceBase.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"

namespace composition
{

    /**
     * @struct LedConfig
     * @brief Configuration for LED device
     */
    struct LedConfig
    {
        int pin = -1; // GPIO pin number (-1 = not configured)
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
    };

    /**
     * @class Led
     * @brief LED with configurable pin, state management
     */
    class Led : public DeviceBase, public ConfigMixin<Led, LedConfig>, public StateMixin<Led, LedState>
    {
    public:
        explicit Led(const String &id);

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        bool set(bool value);
        bool blink(unsigned long onTime = 500, unsigned long offTime = 500);

    protected:
    };

} // namespace composition

#endif // COMPOSITION_LED_H
