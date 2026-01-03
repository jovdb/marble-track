/**
 * @file GpioPin.h
 * @brief Native GPIO pin implementation of IPin interface
 */

#ifndef GPIO_PIN_H
#define GPIO_PIN_H

#include "pins/IPin.h"

namespace pins
{
    /**
     * @class GpioPin
     * @brief Implementation of IPin for native ESP32 GPIO pins
     * 
     * Example usage:
     * @code
     * // In device constructor
     * _pin = new GpioPin(5);  // Create for GPIO 5
     * 
     * // In device setup()
     * _pin->setup(PinMode::Output);
     * 
     * // In device loop() or other methods
     * _pin->write(HIGH);
     * int state = _pin->read();
     * @endcode
     */
    class GpioPin : public IPin
    {
    public:
        /**
         * @brief Construct a new GPIO Pin
         */
        explicit GpioPin();

        /**
         * @brief Set the pin number (can be called before setup)
         * @param pinNumber The GPIO pin number
         */
        void setPinNumber(int pinNumber);

        // IPin interface implementation
        bool setup(PinMode mode) override;
        int read() override;
        bool write(uint8_t value) override;
        int getPinNumber() const override;
        bool isConfigured() const override;
        String toString() const override;

    private:
        int _pinNumber;
        bool _isSetup;
        PinMode _mode;
    };

} // namespace pins

#endif // GPIO_PIN_H
