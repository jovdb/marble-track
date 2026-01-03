/**
 * @file IoExpander.h
 * @brief I2C IO Expander device for adding extra pins via PCF8574, PCF8575, or MCP23017
 */

#ifndef COMPOSITION_IOEXPANDER_H
#define COMPOSITION_IOEXPANDER_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include <Wire.h>

namespace devices
{
    /**
     * @brief Supported I2C expander types
     */
    enum class IoExpanderType
    {
        PCF8574,  // 8-bit I/O expander (pins 0-7)
        PCF8575,  // 16-bit I/O expander (pins 0-15)
        MCP23017  // 16-bit I/O expander with more features (pins 0-15)
    };

    /**
     * @struct IoExpanderConfig
     * @brief Configuration for IO Expander device
     */
    struct IoExpanderConfig
    {
        String name = "IO Expander";          // Device name
        IoExpanderType expanderType = IoExpanderType::PCF8574;
        uint8_t i2cAddress = 0x20;            // Default I2C address
        int sdaPin = 21;                      // I2C SDA pin
        int sclPin = 22;                      // I2C SCL pin
    };

    /**
     * @class IoExpander
     * @brief I2C IO Expander device that provides additional GPIO pins
     * 
     * This device configures an I2C bus and makes expander pins available
     * for use by other devices through the PinFactory.
     */
    class IoExpander : public Device, 
                       public ConfigMixin<IoExpander, IoExpanderConfig>, 
                       public SerializableMixin<IoExpander>
    {
    public:
        explicit IoExpander(const String &id);
        ~IoExpander() override;

        void setup() override;
        void loop() override;
        std::vector<String> getPins() const override;

        /**
         * @brief Check if the I2C device is responding
         * @return true if the device acknowledges on the I2C bus
         */
        bool isDevicePresent() const;

        /**
         * @brief Get the number of pins available on this expander
         * @return Number of pins (8 for PCF8574, 16 for PCF8575/MCP23017)
         */
        int getPinCount() const;

        /**
         * @brief Get the I2C address of this expander
         * @return I2C address
         */
        uint8_t getI2cAddress() const { return _config.i2cAddress; }

        /**
         * @brief Get the expander type
         * @return Expander type
         */
        IoExpanderType getExpanderType() const { return _config.expanderType; }

        /**
         * @brief Get the expander type as string for serialization
         * @return Expander type string
         */
        String getExpanderTypeString() const;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool _isPresent = false;  // Whether the device was found on the I2C bus

        /**
         * @brief Convert string to expander type
         * @param typeStr String representation of expander type
         * @return Expander type enum
         */
        IoExpanderType stringToExpanderType(const String &typeStr) const;
    };

} // namespace devices

#endif // COMPOSITION_IOEXPANDER_H
