/**
 * @file I2c.h
 * @brief I2C bus device for configuring I2C communication
 */

#ifndef I2C_H
#define I2C_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include <Wire.h>

namespace devices
{

    /**
     * @struct I2cConfig
     * @brief Configuration for I2C bus device
     */
    struct I2cConfig
    {
        String name = "I2C";
        int sdaPin = -1;
        int sclPin = -1;
    };

    /**
     * @class I2c
     * @brief I2C bus device for configuring I2C communication
     */
    class I2c : public Device, public ConfigMixin<I2c, I2cConfig>, public SerializableMixin<I2c>
    {
    public:
        explicit I2c(const String &id);

        ~I2c() override;

        void setup() override;
        void teardown() override;
        std::vector<String> getPins() const override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;
    };

} // namespace devices

#endif // I2C_H