/**
 * @file PwdDevice.h
 * @brief PWM Device control using ESP32 MCPWM (Motor Control PWM)
 *
 * This class mirrors the PWM motor control implementation to provide
 * identical functionality under a distinct device type.
 */

#ifndef PWDDEVICE_H
#define PWDDEVICE_H

#include "Device.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

class PwdDevice : public Device
{
public:
    PwdDevice(const String &id, const String &name);

    bool setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits);
    void setDutyCycle(float dutyCycle, bool notifyChange = true);
    void setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);
    float getDutyCycle() const { return _currentDutyCycle; }
    void stop();

    void setup() override;
    void loop() override;
    bool control(const String &action, JsonObject *args) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override;

private:
    int _pin;
    int _pwmChannel;
    uint32_t _frequency;
    uint8_t _resolutionBits;
    float _currentDutyCycle;
    bool _isSetup;
    bool _isAnimating;
    float _startDutyCycle;
    float _targetDutyCycle;
    uint32_t _animationStartTime;
    uint32_t _animationDuration;
    mcpwm_unit_t _mcpwmUnit;
    mcpwm_timer_t _mcpwmTimer;
    mcpwm_io_signals_t _mcpwmSignal;

    bool configureMCPWM();
    void updateAnimation();
    float easeInOutQuad(float t);
};

#endif // PWDDEVICE_H
