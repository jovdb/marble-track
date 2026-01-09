#ifndef PINS_H
#define PINS_H

#include <ArduinoJson.h>
#include "IPin.h"
#include "GpioPin.h"
#include "I2cExpanderPin.h"

enum class PinType
{
    GPIO,         // Native ESP32 GPIO pin
    I2C_PCF8574,  // PCF8574 I2C expander
    I2C_PCF8575,  // PCF8575 I2C expander
    I2C_MCP23017  // MCP23017 I2C expander
};

struct PinConfig
{
    PinType pinType = PinType::GPIO;
    uint8_t i2cAddress = 0x20;
    int pin = -1;
};

class PinFactory
{
public:
    static pins::IPin* createPin(const PinConfig& config);
    static PinConfig jsonToConfig(const JsonDocument& doc);
    static void configToJson(const PinConfig& config, JsonDocument& doc);
    // For backward compatibility, create from int
    static pins::IPin* createPin(int pinNumber);
};

#endif // PINS_H