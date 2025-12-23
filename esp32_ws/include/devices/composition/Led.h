/**
 * @file Led.h
 * @brief Simple LED device using only DeviceBase (no mixins)
 */

#ifndef COMPOSITION_LED_H
#define COMPOSITION_LED_H

#include "devices/composition/DeviceBase.h"

namespace composition
{

/**
 * @class Led
 * @brief Simple LED with hardcoded pin 15
 */
class Led : public DeviceBase
{
public:
    explicit Led(const String &id);

    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    bool set(bool value);
    bool blink(unsigned long onTime = 500, unsigned long offTime = 500);

private:
    enum class Mode
    {
        OFF,
        ON,
        BLINKING
    };

    const int _pin = 15;
    bool _isOn = false;
    unsigned long _blinkOnTime = 500;
    unsigned long _blinkOffTime = 500;
    Mode _mode = Mode::OFF;
};

} // namespace composition

#endif // COMPOSITION_LED_H
