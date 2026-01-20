#ifndef PINS_H
#define PINS_H

#include <ArduinoJson.h>
#include "IPin.h"
#include "GpioPin.h"
#include "I2cExpanderPin.h"

struct PinConfig
{
    String expanderId = "";
    int pin = -1;

    String toString() const
    {
        if (expanderId.isEmpty())
        {
            return "GPIO:" + String(pin);
        }
        else
        {
            return expanderId + ":" + String(pin);
        }
    }
};

class PinFactory
{
public:
    static void setup();
    static pins::IPin *createPin(const PinConfig &config);
    static PinConfig jsonToConfig(const JsonDocument &doc);
    static void configToJson(const PinConfig &config, JsonDocument &doc);
    // For backward compatibility, create from int
    static pins::IPin *createPin(int pinNumber);
};

#endif // PINS_H