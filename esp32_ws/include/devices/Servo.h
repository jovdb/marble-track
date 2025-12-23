/**
 * @file Servo.h
 * @brief Servo motor control using ESP32 MCPWM (Motor Control PWM)
 *
 * This class provides servo control functionality using the ESP32's dedicated
 * Motor Control PWM (MCPWM) peripheral for precise PWM control.
 * Extends ControllableTaskDevice for task-based operation.
 */

#ifndef SERVO_H
#define SERVO_H

#include <atomic>
#include "devices/ControllableTaskDevice.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "McPwmChannels.h"

class Servo : public ControllableTaskDevice
{
public:
    /**
     * @brief Constructor for Servo
     * @param id Unique identifier for the servo
     * @param callback Callback function for notifying clients
     */
    Servo(const String &id, NotifyClients callback = nullptr);

    /**
     * @brief Destructor for Servo
     */
    virtual ~Servo();

    // ControllableTaskDevice interface implementations
    void getConfigFromJson(const JsonDocument &config) override;
    void addStateToJson(JsonDocument &doc) override;
    void addConfigToJson(JsonDocument &doc) const override;
    bool control(const String &action, JsonObject *args = nullptr) override;
    std::vector<int> getPins() const override;

    /**
     * @brief Set servo value using normalized 0.0-1.0 range
     * @param value Normalized value (0.0 = min duty cycle, 1.0 = max duty cycle)
     * @param durationMs Optional duration for animation (default: uses configured default duration)
     * @return true if set successfully, false if not configured
     */
    bool setValue(float value, int durationMs = -1);

    /**
     * @brief Get current value as percentage (0-100)
     * @return Current value as percentage, calculated from duty cycle range
     */
    float getValue() const;

    /**
     * @brief Stop the servo animation
     */
    void stop();

protected:
    void task() override;

private:
    // Configuration
    String _name;
    int _pin;
    int _mcpwmChannelIndex;
    uint32_t _frequency;
    uint8_t _resolutionBits;
    float _minDutyCycle;
    float _maxDutyCycle;
    uint32_t _defaultDurationInMs;
    bool _isSetup;

    // Thread-safe state variables
    std::atomic<float> _currentDutyCycle{0.0f};
    std::atomic<bool> _isAnimating{false};
    std::atomic<float> _startDutyCycle{0.0f};
    std::atomic<float> _targetDutyCycle{0.0f};
    std::atomic<uint32_t> _animationStartTime{0};
    std::atomic<uint32_t> _animationDuration{0};

    // MCPWM configuration
    mcpwm_unit_t _mcpwmUnit;
    mcpwm_timer_t _mcpwmTimer;
    mcpwm_io_signals_t _mcpwmSignal;
    mcpwm_operator_t _mcpwmOperator;

    /**
     * @brief Setup the PWM servo with specified parameters
     * @return true if setup successful, false otherwise
     */
    bool setupServo();

    /**
     * @brief Configure MCPWM for servo control
     * @return true if configuration successful
     */
    bool configureMCPWM();

    /**
     * @brief Set servo duty cycle
     * @param dutyCycle Duty cycle as percentage (0.0 to 100.0)
     * @param notifyChange Whether to send state change notification (default: true)
     * @return true if set successfully, false if not configured
     */
    bool setDutyCycle(float dutyCycle, bool notifyChange = true);

    /**
     * @brief Set servo duty cycle with animated transition
     * @param dutyCycle Target duty cycle as percentage (0.0 to 100.0)
     * @param durationMs Duration of animation in milliseconds
     * @return true if animation started, false if not configured
     */
    bool setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);

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

#endif // SERVO_H
