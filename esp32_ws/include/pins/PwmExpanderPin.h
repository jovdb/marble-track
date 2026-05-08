/**
 * @file PwmExpanderPin.h
 * @brief PCA9685 16-channel 12-bit PWM expander pin implementation of IPin interface
 */

#ifndef PWM_EXPANDER_PIN_H
#define PWM_EXPANDER_PIN_H

#include "pins/IPin.h"
#include <Adafruit_PWMServoDriver.h>

namespace pins
{
    /**
     * @class PwmExpanderPin
     * @brief Implementation of IPin for a single channel on a PCA9685 PWM expander
     *
     * write(0)     → channel fully off
     * write(> 0)   → channel fully on (12-bit 4095)
     *
     * For proportional PWM (0-4095) callers may cast to PwmExpanderPin and call
     * writePwm() directly.
     */
    class PwmExpanderPin : public IPin
    {
    public:
        /**
         * @brief Construct a PwmExpanderPin
         * @param driver  Shared Adafruit_PWMServoDriver instance (must outlive this pin)
         * @param expanderId  ID of the owning PwmExpander device (for toString)
         */
        explicit PwmExpanderPin(Adafruit_PWMServoDriver *driver, const String &expanderId);

        // IPin interface
        bool setup(int pinNumber, PinMode mode) override;
        int  read() override;
        bool write(uint8_t value) override;
        int  getPinNumber() const override;
        bool isConfigured() const override;
        String toString() const override;

        /**
         * @brief Write a raw 12-bit PWM duty cycle (0-4095)
         * @param value 0 = fully off, 4095 = fully on
         */
        bool writePwm(uint16_t value);

    private:
        Adafruit_PWMServoDriver *_driver;
        String _expanderId;
        int    _channel;
        bool   _isSetup;
    };

} // namespace pins

#endif // PWM_EXPANDER_PIN_H
