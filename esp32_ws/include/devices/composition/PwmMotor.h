/**
 * @file PwmMotor.h
 * @brief PWM Motor device using DeviceBase with composition mixins
 */

#ifndef COMPOSITION_PWM_MOTOR_H
#define COMPOSITION_PWM_MOTOR_H

#include "devices/composition/DeviceBase.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "McPwmChannels.h"

namespace devices
{

    /**
     * @struct PwmMotorConfig
     * @brief Configuration for PwmMotor device
     */
    struct PwmMotorConfig
    {
        String name = "PwmMotor";     // Device name
        int pin = -1;                 // GPIO pin number
        int mcpwmChannel = -1;        // MCPWM channel index (0-5), or -1 to auto-acquire
        uint32_t frequency = 1000;    // PWM frequency in Hz
        uint8_t resolutionBits = 8;   // PWM resolution in bits (8-16)
        float minDutyCycle = 0.0f;    // Minimum duty cycle percentage
        float maxDutyCycle = 100.0f;  // Maximum duty cycle percentage
        uint32_t defaultDurationInMs = 0; // Default animation duration
    };

    /**
     * @struct PwmMotorState
     * @brief State structure for PwmMotor device
     */
    struct PwmMotorState
    {
        float currentDutyCycle = 0.0f; // Current duty cycle as percentage
        bool isAnimating = false;      // Whether animation is in progress
        float targetDutyCycle = 0.0f;  // Target duty cycle for animation
        uint32_t animationStartTime = 0; // Animation start time
        uint32_t animationDuration = 0;  // Animation duration
    };

    /**
     * @class PwmMotor
     * @brief PWM Motor control using ESP32 MCPWM with composition pattern
     */
    class PwmMotor : public DeviceBase,
                     public ConfigMixin<PwmMotor, PwmMotorConfig>,
                     public StateMixin<PwmMotor, PwmMotorState>,
                     public ControllableMixin<PwmMotor>,
                     public SerializableMixin<PwmMotor>
    {
    public:
        explicit PwmMotor(const String &id);
        ~PwmMotor();

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        /**
         * @brief Set motor duty cycle
         * @param dutyCycle Duty cycle as percentage (0.0 to 100.0)
         * @param notifyChange Whether to send state change notification (default: true)
         * @return true if set successfully, false if not configured
         */
        bool setDutyCycle(float dutyCycle, bool notifyChange = true);

        /**
         * @brief Set motor duty cycle with animated transition
         * @param dutyCycle Target duty cycle as percentage (0.0 to 100.0)
         * @param durationMs Duration of animation in milliseconds
         * @return true if animation started, false if not configured
         */
        bool setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);

        /**
         * @brief Set motor value using normalized 0.0-1.0 range
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
         * @brief Get current duty cycle
         * @return Current duty cycle as percentage (0.0 to 100.0)
         */
        float getDutyCycle() const { return _state.currentDutyCycle; }

        /**
         * @brief Stop the motor (set duty cycle to 0)
         */
        void stop();

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool _isSetup = false;
        mcpwm_unit_t _mcpwmUnit;
        mcpwm_timer_t _mcpwmTimer;
        mcpwm_io_signals_t _mcpwmSignal;
        mcpwm_operator_t _mcpwmOperator;

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

} // namespace devices

#endif // COMPOSITION_PWM_MOTOR_H
