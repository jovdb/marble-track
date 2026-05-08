/**
 * @file PwmExpander.h
 * @brief PCA9685 16-channel 12-bit PWM expander device
 */

#ifndef COMPOSITION_PWMEXPANDER_H
#define COMPOSITION_PWMEXPANDER_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

namespace devices
{
    /**
     * @struct PwmExpanderConfig
     * @brief Configuration for PwmExpander device (PCA9685)
     */
    struct PwmExpanderConfig
    {
        String  name        = "PWM Expander"; // Device name
        String  i2cDeviceId = "";             // ID of the I2C bus device to use
        uint8_t i2cAddress  = 0x40;           // Default PCA9685 I2C address
        float   frequency   = 50.0f;          // PWM frequency in Hz (50 = servo, up to ~1526)
    };

    /**
     * @class PwmExpander
     * @brief PCA9685 16-channel 12-bit PWM expander device
     *
     * Registers itself so that PinFactory can create PwmExpanderPin instances
     * for channels 0-15 on this device.
     */
    class PwmExpander : public Device,
                        public ConfigMixin<PwmExpander, PwmExpanderConfig>,
                        public SerializableMixin<PwmExpander>
    {
    public:
        explicit PwmExpander(const String &id);
        ~PwmExpander() override;

        void setup() override;
        void teardown() override;
        void loop() override;
        std::vector<String> getPins() const override;

        /** @brief Whether the PCA9685 responded on the I2C bus during setup */
        bool isDevicePresent() const;

        /** @brief Always 16 channels on a PCA9685 */
        int getPinCount() const { return 16; }

        /** @brief I2C address of this expander */
        uint8_t getI2cAddress() const { return _config.i2cAddress; }

        /**
         * @brief Get a pointer to the underlying driver (used by PinFactory)
         * @return Pointer to Adafruit_PWMServoDriver, or nullptr if not set up
         */
        Adafruit_PWMServoDriver *getDriver() { return _driver; }

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool _isPresent = false;
        Adafruit_PWMServoDriver *_driver = nullptr;
    };

} // namespace devices

#endif // COMPOSITION_PWMEXPANDER_H
