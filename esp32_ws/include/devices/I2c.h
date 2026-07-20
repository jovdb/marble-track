/**
 * @file I2c.h
 * @brief I2C bus device for configuring I2C communication
 */

#ifndef I2C_H
#define I2C_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/StateMixin.h"
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
     * @struct I2cState
     * @brief State structure for I2C bus device
     */
    struct I2cState
    {
        // "Ready" = bus initialized with valid SDA/SCL pins
        // "Error" = pins missing or invalid
        String state = "Error";
        std::vector<int> foundAddresses;

        // Results of the last manual operation
        struct LastOp
        {
            String type;      // "write" or "read"
            int address = 0;
            String data;      // Data sent/received (hex)
            int length = 0;   // Length requested (for read)
            String status;    // "OK", "NACK", "Timeout", etc.
            uint32_t timestamp = 0;
        } lastOp;
    };

    /**
     * @class I2c
     * @brief I2C bus device for configuring I2C communication
     */
    class I2c : public Device,
                 public ConfigMixin<I2c, I2cConfig>,
                 public StateMixin<I2c, I2cState>,
                 public ControllableMixin<I2c>,
                 public SerializableMixin<I2c>
    {
    public:
        explicit I2c(const String &id);

        ~I2c() override;

        void setup() override;
        void teardown() override;
        std::vector<String> getPins() const override;

        /**
         * @brief Scans the I2C bus for devices
         * @return A vector of found I2C addresses
         */
        std::vector<int> scanBus();

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;
    };

} // namespace devices

#endif // I2C_H