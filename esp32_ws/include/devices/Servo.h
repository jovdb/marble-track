/**
 * @file Servo.h
 * @brief Servo device using DeviceBase with composition mixins
 */

#ifndef COMPOSITION_SERVO_H
#define COMPOSITION_SERVO_H

#include "devices/DeviceBase.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/RtosMixin.h"
#include "McPwmChannels.h"
#include <atomic>
#include <freertos/semphr.h>

namespace devices
{

    /**
     * @struct ServoConfig
     * @brief Configuration for Servo device
     */
    struct ServoConfig
    {
        int pin = -1;                    // GPIO pin number (-1 = not configured)
        String name = "Servo";           // Device name
        int mcpwmChannel = -1;           // MCPWM channel (-1 = auto-assign)
        uint32_t frequency = 50;         // PWM frequency in Hz
        uint8_t resolutionBits = 10;     // PWM resolution bits
        float minDutyCycle = 2.5f;       // Minimum duty cycle percentage
        float maxDutyCycle = 12.5f;      // Maximum duty cycle percentage
        uint32_t defaultDurationInMs = 500; // Default animation duration
    };

    /**
     * @struct ServoState
     * @brief State structure for Servo device
     */
    struct ServoState
    {
        bool running = false;        // True if animation is in progress
        float value = 0.0f;          // Current position as percentage (0-100)
        float targetValue = 0.0f;    // Target position as percentage (0-100)
        uint32_t targetDurationMs = 0; // Remaining animation time in ms
    };

    /**
     * @class Servo
     * @brief Servo with configurable pin, position control, and animation
     */
    class Servo : public DeviceBase,
                  public ConfigMixin<Servo, ServoConfig>,
                  public StateMixin<Servo, ServoState>,
                  public ControllableMixin<Servo>,
                  public SerializableMixin<Servo>,
                  public RtosMixin<Servo>
    {
    public:
        explicit Servo(const String &id);
        ~Servo();

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        /**
         * @brief Set servo position with optional animation
         * @param value Position as normalized value (0.0-1.0)
         * @param durationMs Animation duration in milliseconds (-1 for default)
         * @return true if successful, false otherwise
         */
        bool setValue(float value, int durationMs = -1);

        /**
         * @brief Stop any current animation
         * @return true if stopped, false if not animating
         */
        bool stop();

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

        // RTOS task implementation
        void task() override;

    private:
        /**
         * @brief Setup MCPWM for servo control
         * @return true if successful, false otherwise
         */
        bool setupServo();

        /**
         * @brief Configure MCPWM parameters
         * @return true if successful, false otherwise
         */
        bool configureMCPWM();

        /**
         * @brief Set duty cycle immediately
         * @param dutyCycle Duty cycle percentage (0-100)
         * @param notifyChange Whether to notify state change
         * @return true if successful, false otherwise
         */
        bool setDutyCycle(float dutyCycle, bool notifyChange = true);

        /**
         * @brief Set duty cycle with animation
         * @param dutyCycle Duty cycle percentage (0-100)
         * @param durationMs Animation duration in milliseconds
         * @return true if successful, false otherwise
         */
        bool setDutyCycleAnimated(float dutyCycle, uint32_t durationMs);

        /**
         * @brief Update animation progress
         */
        void updateAnimation();

        /**
         * @brief Ease-in-out quadratic function for smooth animation
         * @param t Progress value (0.0-1.0)
         * @return Eased progress value
         */
        float easeInOutQuad(float t);

        // MCPWM configuration
        int _mcpwmChannelIndex = -1;
        mcpwm_unit_t _mcpwmUnit = MCPWM_UNIT_0;
        mcpwm_timer_t _mcpwmTimer = MCPWM_TIMER_0;
        mcpwm_operator_t _mcpwmOperator = MCPWM_OPR_A;
        mcpwm_io_signals_t _mcpwmSignal = MCPWM0A;

        // Animation state (thread-safe with atomics)
        std::atomic<float> _currentDutyCycle = 0.0f;
        std::atomic<float> _startDutyCycle = 0.0f;
        std::atomic<float> _targetDutyCycle = 0.0f;
        std::atomic<uint32_t> _animationStartTime = 0;
        std::atomic<uint32_t> _animationDuration = 0;
        std::atomic<bool> _isAnimating = false;

        bool _isSetup = false;
        SemaphoreHandle_t _stateMutex; // Mutex for thread-safe state access
    };

} // namespace devices

#endif // COMPOSITION_SERVO_H
