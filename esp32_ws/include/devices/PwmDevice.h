#ifndef PWMDEVICE_H
#define PWMDEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Device.h"

/**
 * @class PwmDevice
 * @brief PWM signal control class
 *
 * Provides PWM signal generation using ESP32 LEDC peripheral.
 */
class PwmDevice : public Device
{
public:
    /**
     * @brief Constructor - automatically initializes with ID only
     * @param id Unique identifier string for the PWM device
     */
    PwmDevice(const String &id, NotifyClients callback = nullptr);

    /**
     * @brief Destructor
     */
    ~PwmDevice();

    void setup() override;
    void loop() override;

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override;

    // PWM-specific operations
    bool setDutyCycle(int dutyCycle);

private:
    int _pin = -1;
    int _channel = -1;
    int _frequency = 5000;
    int _resolution = 8;
    int _dutyCycle = 0;

    static int _nextChannel;
};

#endif // PWMDEVICE_H