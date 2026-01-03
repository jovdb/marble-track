/**
 * @file IPin.h
 * @brief Abstract pin interface for GPIO abstraction
 * 
 * This abstraction layer allows devices to work with both native GPIO pins
 * and I2C expansion board pins (like MCP23017, PCF8574) through a common interface.
 */

#ifndef IPIN_H
#define IPIN_H

#include <Arduino.h>

namespace pins
{
    /**
     * @brief Pin mode options for input configuration
     */
    enum class PinMode
    {
        Input,
        InputPullUp,
        InputPullDown,
        Output
    };

    /**
     * @class IPin
     * @brief Abstract base class for pin operations
     * 
     * Provides a common interface for GPIO operations that can be implemented
     * by different pin sources (native GPIO, I2C expanders, etc.)
     * 
     * Usage pattern:
     * 1. Create pin instance in device constructor
     * 2. Call setup() in device's setup() method
     * 3. Use read()/write() in device loop() and other methods
     */
    class IPin
    {
    public:
        virtual ~IPin() = default;

        /**
         * @brief Setup the pin with the specified mode
         * @param mode The pin mode (Input, InputPullUp, InputPullDown, Output)
         * @return true if setup was successful
         */
        virtual bool setup(PinMode mode) = 0;

        /**
         * @brief Read the current state of the pin
         * @return HIGH (1) or LOW (0), or -1 on error
         */
        virtual int read() = 0;

        /**
         * @brief Write a value to the pin
         * @param value HIGH or LOW
         * @return true if write was successful
         */
        virtual bool write(uint8_t value) = 0;

        /**
         * @brief Get the pin number/identifier for logging purposes
         * @return Pin number or identifier
         */
        virtual int getPinNumber() const = 0;

        /**
         * @brief Check if the pin is configured/valid
         * @return true if the pin is properly configured
         */
        virtual bool isConfigured() const = 0;

        /**
         * @brief Get a string description of the pin for logging
         * @return String like "GPIO:5" or "MCP23017:0x20:3"
         */
        virtual String toString() const = 0;
    };

} // namespace pins

#endif // IPIN_H
