/**
 * @file PwmMotor.h
 * @brief PWM Motor control using ESP32 MCPWM (Motor Control PWM)
 *
 * This class provides motor control functionality using the ESP32's dedicated
 * Motor Control PWM (MCPWM) peripheral for precise motor control.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef PWMMOTOR_H
#define PWMMOTOR_H

#include "Device.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

class PwmMotor : public Device
{
public:
    /**
     * @brief Constructor for PwmMotor
     * @param id Unique identifier for the motor
     * @param name Human-readable name for the motor
     */
    PwmMotor(const String &id, const String &name);

    /**
     * @brief Setup the PWM motor with specified parameters
     * @param pin GPIO pin number for PWM output
     * @param pwmChannel PWM channel (0 or 1)
     * @param frequency PWM frequency in Hz
     * @param resolutionBits PWM resolution in bits (8-16)
     * @return true if setup successful, false otherwise
     */
    bool setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits);

    /**
     * @brief Set motor duty cycle
     * @param dutyCycle Duty cycle as percentage (0.0 to 100.0)
     * @param notifyChange Whether to send state change notification (default: true)
     */
    void setDutyCycle(float dutyCycle, bool notifyChange = true);

    /**
     * @brief Set motor duty cycle with animated transition
     * @param dutyCycle Target duty cycle as percentage (0.0 to 100.0)
     * @param durationMs Duration of animation in milliseconds
     */
    void setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);

    /**
     * @brief Get current duty cycle
     * @return Current duty cycle as percentage (0.0 to 100.0)
     */
    float getDutyCycle() const { return _currentDutyCycle; }

    /**
     * @brief Stop the motor (set duty cycle to 0)
     */
    void stop();

    // Device interface implementations
    void setup() override;
    void loop() override;
    bool control(const String &action, JsonObject *args) override;
    String getState() override;

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

    /**
     * @brief Configure MCPWM for motor control
     * @return true if configuration successful
     */
    bool configureMCPWM();

    /**
     * @brief Update animation state and apply eased duty cycle
     */
    void updateAnimation();

    /**
     * @brief Easing function for smooth animations (ease-in-out)
     * @param t Normalized time (0.0 to 1.0)
     * @return Eased value (0.0 to 1.0)
     */
    float easeInOutQuad(float t);
};

#endif // PWMMOTOR_H