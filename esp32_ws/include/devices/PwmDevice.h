/**
 * @file PwmDevice.h
 * @brief Generic PWM control using ESP32 MCPWM (Motor Control PWM)
 *
 * This class mirrors the PwmMotor implementation but is exposed separately so the
 * firmware and UI can evolve the behaviours independently.
 */

#ifndef PWM_DEVICE_H
#define PWM_DEVICE_H

#include "Device.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

class PwmDevice : public Device
{
public:
    /**
     * @brief Constructor for PwmDevice
     * @param id Unique identifier for the PWM output
     * @param name Human-readable name for the PWM output
     */
    PwmDevice(const String &id, const String &name);

    /**
     * @brief Setup the PWM output with specified parameters
     * @param pin GPIO pin number for PWM output
     * @param pwmChannel PWM channel (0 or 1)
     * @param frequency PWM frequency in Hz
     * @param resolutionBits PWM resolution in bits (8-16)
     * @return true if setup successful, false otherwise
     */
    bool setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits);

    /**
     * @brief Set duty cycle immediately
     * @param dutyCycle Duty cycle percentage (0.0 to 100.0)
     * @param notifyChange Whether to broadcast state change (default: true)
     * @return true if set successfully, false if not configured
     */
    bool setDutyCycle(float dutyCycle, bool notifyChange = true);

    /**
     * @brief Animate duty cycle towards a target value
     * @param dutyCycle Target duty cycle percentage (0.0 to 100.0)
     * @param durationMs Duration of the animation in milliseconds
     * @return true if animation started, false if not configured
     */
    bool setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);

    /**
     * @brief Current duty cycle percentage
     */
    float getDutyCycle() const { return _currentDutyCycle; }

    /**
     * @brief Stop output (sets duty cycle to 0)
     */
    void stop();

    // Device interface implementations
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

    // Animation state
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

#endif // PWM_DEVICE_H
