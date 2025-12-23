/**
 * @file Led.h
 * @brief Simple LED device using only DeviceBase (no mixins)
 */

#ifndef COMPOSITION_LED_H
#define COMPOSITION_LED_H

#include "devices/composition/DeviceBase.h"
#include "devices/composition/StateMixin.h"

namespace composition
{

/**
 * @struct LedState
 * @brief State structure for LED device
 */
struct LedState
{
    String mode = "OFF";  // OFF, ON, or BLINKING
    unsigned long blinkOnTime = 500;
    unsigned long blinkOffTime = 500;
};

/**
 * @class Led
 * @brief Simple LED with hardcoded pin 15 and state management
 */
class Led : public DeviceBase, public StateMixin<Led, LedState>
{
public:
    explicit Led(const String &id);

    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    bool set(bool value);
    bool blink(unsigned long onTime = 500, unsigned long offTime = 500);

private:
    const int _pin = 15;
};

} // namespace composition

#endif // COMPOSITION_LED_H
