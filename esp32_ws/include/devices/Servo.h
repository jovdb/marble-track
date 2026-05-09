/**
 * @file Servo.h
 * @brief Servo device using Device with composition mixins
 */

#ifndef COMPOSITION_SERVO_H
#define COMPOSITION_SERVO_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "McPwmChannels.h"
#include "pins/Pins.h"
#include "pins/PwmExpanderPin.h"

namespace devices
{

    /**
     * @struct ServoConfig
     * @brief Configuration for Servo device
     */
    struct ServoConfig
    {
        PinConfig pinConfig;             // Pin configuration (GPIO or PwmExpander channel)
        String name = "Servo";           // Device name
        int mcpwmChannel = -1;           // MCPWM channel (-1 = auto-assign, ignored for PwmExpander)
        uint32_t frequency = 50;         // PWM frequency in Hz (ignored for PwmExpander)
        uint8_t resolutionBits = 10;     // PWM resolution bits (ignored for PwmExpander)
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
    class Servo : public Device,
                  public ConfigMixin<Servo, ServoConfig>,
                  public StateMixin<Servo, ServoState>,
                  public ControllableMixin<Servo>,
                  public SerializableMixin<Servo>
    {
    public:
        explicit Servo(const String &id);
        ~Servo();

        void setup() override;
        void teardown() override;
        void loop() override;
        std::vector<String> getPins() const override;

        /**
         * @brief Set servo position
         * @param value Position as normalized value (0.0-1.0)
         * @param durationMs Transition duration in milliseconds (-1 uses configured default)
         * @return true if successful, false otherwise
         */
        bool setValue(float value, int durationMs = -1);

        /**
         * @brief Stop any current animation
         * @return true when an animation was active and stopped, false otherwise
         */
        bool stop();

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        /**
         * @brief Setup MCPWM for servo control
         * @return true if successful, false otherwise
         */
        bool setupServo();

        /**
         * @brief Setup PwmExpander channel for servo control
         * @return true if successful, false otherwise
         */
        bool setupPwmExpander();

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

        // MCPWM configuration (used only when pin is a plain GPIO pin)
        int _mcpwmChannelIndex = -1;
        mcpwm_unit_t _mcpwmUnit = MCPWM_UNIT_0;
        mcpwm_timer_t _mcpwmTimer = MCPWM_TIMER_0;
        mcpwm_operator_t _mcpwmOperator = MCPWM_OPR_A;
        mcpwm_io_signals_t _mcpwmSignal = MCPWM0A;

        // PwmExpander pin (used when pinConfig.expanderId is set)
        pins::PwmExpanderPin *_pwmPin = nullptr;

        // State
        float _currentDutyCycle = 0.0f;
        bool _isAnimating = false;
        float _animationStartDutyCycle = 0.0f;
        float _animationTargetDutyCycle = 0.0f;
        uint32_t _animationStartTimeMs = 0;
        uint32_t _animationDurationMs = 0;

        bool _isSetup = false;
        bool _wasAutoAssigned = false; // Flag to track if channel was auto-assigned
    };

} // namespace devices

#endif // COMPOSITION_SERVO_H
