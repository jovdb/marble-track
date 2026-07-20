/**
 * @file Adxl345.h
 * @brief ADXL345 3-axis accelerometer device
 */

#ifndef COMPOSITION_ADXL345_H
#define COMPOSITION_ADXL345_H

#include "devices/Device.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/StateMixin.h"
#include <Adafruit_ADXL345_U.h>

namespace devices
{

    struct Adxl345Config
    {
        String name = "Accelerometer";
        String i2cDeviceId = "";             // ID of the I2C bus device to use
        uint8_t i2cAddress = 0x53;           // ADXL345 I2C address (usually 0x53 or 0x1D)
        int range = 16;                      // 2, 4, 8, 16 G
        unsigned long refreshIntervalMs = 100; // How often to read from sensor
    };

    struct Adxl345State
    {
        String status = "Error";  // "Ready" | "Error"
        float x = 0.0f;           // X-axis acceleration in m/s^2
        float y = 0.0f;           // Y-axis acceleration in m/s^2
        float z = 0.0f;           // Z-axis acceleration in m/s^2
        unsigned long lastUpdatedMillis = 0; // Timestamp of last reading
    };

    class Adxl345 : public Device,
                    public ConfigMixin<Adxl345, Adxl345Config>,
                    public StateMixin<Adxl345, Adxl345State>,
                    public ControllableMixin<Adxl345>,
                    public SerializableMixin<Adxl345>
    {
    public:
        explicit Adxl345(const String &id);
        ~Adxl345() override;

        void setup() override;
        void teardown() override;
        void loop() override;

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &doc) override;
        void configToJson(JsonDocument &doc) override;

        void readSensor();

    private:
        Adafruit_ADXL345_Unified *_adxl = nullptr;
        void init();
    };

} // namespace devices

#endif // COMPOSITION_ADXL345_H
