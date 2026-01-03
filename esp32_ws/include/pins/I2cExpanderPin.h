/**
 * @file I2cExpanderPin.h
 * @brief I2C GPIO expander pin implementation of IPin interface
 * 
 * Supports common I2C GPIO expanders like PCF8574 (8-bit) and MCP23017 (16-bit).
 * Uses the Wire library for I2C communication.
 */

#ifndef I2C_EXPANDER_PIN_H
#define I2C_EXPANDER_PIN_H

#include "pins/IPin.h"
#include <Wire.h>

namespace pins
{
    /**
     * @brief Supported I2C GPIO expander types
     */
    enum class I2cExpanderType
    {
        PCF8574,   // 8-bit I/O expander (pins 0-7)
        PCF8575,   // 16-bit I/O expander (pins 0-15)
        MCP23017   // 16-bit I/O expander with more features (pins 0-15)
    };

    /**
     * @class I2cExpanderPin
     * @brief Implementation of IPin for I2C GPIO expander pins
     * 
     * This class manages individual pins on I2C GPIO expanders. Multiple instances
     * can share the same physical expander chip by using the same I2C address.
     * 
     * Note: For efficiency, this class caches the port state and only writes
     * when the pin value actually changes.
     * 
     * Example usage:
     * @code
     * // In device constructor
     * _pin = new I2cExpanderPin(I2cExpanderType::PCF8574, 0x20);
     * 
     * // In device setup()
     * _pin->setup(3, PinMode::Output);  // Pin 3 on the expander
     * 
     * // In device loop() or other methods
     * _pin->write(HIGH);
     * int state = _pin->read();
     * @endcode
     */
    class I2cExpanderPin : public IPin
    {
    public:
        /**
         * @brief Construct a new I2C Expander Pin
         * @param type The type of I2C expander chip
         * @param i2cAddress The I2C address of the expander (default 0x20)
         * @param wireInstance Pointer to Wire instance (default &Wire)
         */
        explicit I2cExpanderPin(I2cExpanderType type = I2cExpanderType::PCF8574, 
                                uint8_t i2cAddress = 0x20,
                                TwoWire* wireInstance = &Wire);

        // IPin interface implementation
        bool setup(int pinNumber, PinMode mode) override;
        int read() override;
        bool write(uint8_t value) override;
        int getPinNumber() const override;
        bool isConfigured() const override;
        String toString() const override;

        /**
         * @brief Get the I2C address of the expander
         * @return The I2C address
         */
        uint8_t getI2cAddress() const { return _i2cAddress; }

        /**
         * @brief Get the expander type
         * @return The expander type
         */
        I2cExpanderType getExpanderType() const { return _expanderType; }

        /**
         * @brief Check if the I2C device is responding
         * @return true if the device acknowledges on the I2C bus
         */
        bool isDevicePresent();

    private:
        I2cExpanderType _expanderType;
        uint8_t _i2cAddress;
        TwoWire* _wire;
        int _pinNumber;
        bool _isSetup;
        PinMode _mode;
        
        // Static port state cache per I2C address (shared across all pins on same expander)
        // Key: (expanderType << 8) | i2cAddress, Value: port state
        static uint16_t _portStates[16];  // Support up to 16 different expanders
        static uint16_t _portDirections[16];  // Direction registers for MCP23017
        
        /**
         * @brief Get the cache index for this expander
         * @return Index into the static port state arrays
         */
        uint8_t getCacheIndex() const;
        
        /**
         * @brief Read the current port state from the expander
         * @return The port state (8 or 16 bits depending on expander type)
         */
        uint16_t readPort();
        
        /**
         * @brief Write the port state to the expander
         * @param state The port state to write
         * @return true if successful
         */
        bool writePort(uint16_t state);
        
        /**
         * @brief Configure the direction register (MCP23017 only)
         * @return true if successful
         */
        bool configureDirection();
        
        /**
         * @brief Get the maximum pin number for this expander type
         * @return Maximum pin number (7 for 8-bit, 15 for 16-bit expanders)
         */
        int getMaxPinNumber() const;
        
        /**
         * @brief Get the expander type name for logging
         * @return String name of the expander type
         */
        String getExpanderTypeName() const;
    };

} // namespace pins

#endif // I2C_EXPANDER_PIN_H
