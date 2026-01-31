/**
 * @file Lift.h
 * @brief Lift device using Device with composition mixins
 */

#ifndef COMPOSITION_LIFT_H
#define COMPOSITION_LIFT_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "devices/Servo.h"

namespace devices
{

    /**
     * @enum LiftStateEnum
     * @brief Enumeration of possible lift states
     */
    enum class LiftStateEnum
    {
        UNKNOWN,
        ERROR,
        INIT,
        LIFT_DOWN_LOADING,
        LIFT_DOWN,
        LIFT_UP_UNLOADING,
        LIFT_UP,
        MOVING_UP,
        MOVING_DOWN
    };

    /**
     * @enum LiftErrorCode
     * @brief Enumeration of possible lift error codes
     */
    enum class LiftErrorCode
    {
        NONE,
        LIFT_CONFIGURATION_ERROR,
        LIFT_STATE_ERROR,
        LIFT_NO_ZERO,
    };

    /**
     * @struct LiftConfig
     * @brief Configuration for Lift device
     */
    struct LiftConfig
    {
        String name = "Lift";      // Device name
        long minSteps = 0;         // Minimum steps (bottom position)
        long maxSteps = 1000;      // Maximum steps (top position)
        float downFactor = 1.015f; // Extra movement factor when going down
    };

    /**
     * @struct LiftState
     * @brief State structure for Lift device
     */
    struct LiftState
    {
        LiftStateEnum state = LiftStateEnum::UNKNOWN;  // Current lift state
        unsigned long ballWaitingSince = 0;            // Timestamp when ball started waiting (0 = not waiting)
        bool isLoaded = false;                         // Whether lift has a ball loaded
        int initStep = 0;                              // Current initialization step
        bool onErrorChange = false;                    // Error flag
        String errorMessage = "";                      // Last error message
        LiftErrorCode errorCode = LiftErrorCode::NONE; // Last error code
    };

    /**
     * @class Lift
     * @brief Lift device with stepper motor and sensor control
     *
     * Manages a lift mechanism with stepper motor positioning, limit switch,
     * ball sensor, and loader/unloader motors.
     * Uses composition pattern with children devices.
     */
    class Lift : public Device,
                 public ConfigMixin<Lift, LiftConfig>,
                 public StateMixin<Lift, LiftState>,
                 public ControllableMixin<Lift>,
                 public SerializableMixin<Lift>
    {
    public:
        explicit Lift(const String &id);
        ~Lift();

        void setup() override;
        void teardown() override;
        void loop() override;

        /**
         * @brief Move the lift up to the top position
         * @param speedRatio Speed ratio (1.0 = default speed)
         * @return true if move initiated, false otherwise
         */
        bool up(float speedRatio = 1.0f);

        /**
         * @brief Move the lift down to the bottom position
         * @param speedRatio Speed ratio (1.0 = default speed)
         * @return true if move initiated, false otherwise
         */
        bool down(float speedRatio = 1.0f);

        /**
         * @brief Initialize the lift (calibration sequence)
         * @return true if init started, false otherwise
         */
        bool init();

        /**
         * @brief Load a ball into the lift
         * @return true if load started, false otherwise
         */
        bool loadBall();

        /**
         * @brief Unload a ball from the lift
         * @param durationRatio Duration ratio for servo animation (1.0 = default duration)
         * @return true if unload started, false otherwise
         */
        bool unloadBall(float durationRatio = 1.0f);

        /**
         * @brief Check if a ball is waiting to be loaded
         * @return true if ball is waiting
         */
        bool isBallWaiting() const;

        /**
         * @brief Check if the lift has a ball loaded
         * @return true if loaded
         */
        bool isLoaded() const;

        /**
         * @brief Check if the lift is initialized
         * @return true if initialized (not in INIT or UNKNOWN state)
         */
        bool isInitialized() const;

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    protected:
        Stepper *_stepper;    // Stepper motor child device
        Button *_limitSwitch; // Limit switch child device
        Button *_ballSensor;  // Ball sensor child device
        Servo *_loader;       // Loader motor child device
        Servo *_unloader;     // Unloader motor child device

        unsigned long _loadStartTime = 0;    // Load operation start time
        unsigned long _unloadStartTime = 0;  // Unload operation start time
        unsigned long _unloadEndTime = 0;    // Unload operation end time
        unsigned long _stepperStartTime = 0; // Stepper start time (0 when stopped)

    private:
        /**
         * @brief Convert LiftStateEnum to string
         * @param state State enum value
         * @return String representation
         */
        String stateToString(LiftStateEnum state) const;

        /**
         * @brief Convert LiftErrorCode to string
         * @param errorCode Error code enum value
         * @return String representation
         */
        String errorCodeToString(LiftErrorCode errorCode) const;

        /**
         * @brief Start loading a ball
         * @return true if started
         */
        bool loadBallStart();

        /**
         * @brief End loading a ball
         * @return true if ended
         */
        bool loadBallEnd();

        /**
         * @brief Start unloading a ball
         * @param durationRatio Duration ratio for servo animation (1.0 = default duration)
         * @return true if started
         */
        bool unloadBallStart(float durationRatio = 1.0f);

        /**
         * @brief End unloading a ball
         * @param durationRatio Duration ratio for servo animation (1.0 = default duration)
         * @return true if ended
         */
        bool unloadBallEnd(float durationRatio = 1.0f);

        /**
         * @brief Get current stepper position
         * @return Current position
         */
        long getCurrentPosition() const;

        /**
         * @brief Move stepper by steps
         * @param steps Number of steps
         * @param speedRatio Speed ratio
         * @return true if moved
         */
        bool moveStepper(long steps, float speedRatio);

        /**
         * @brief Move stepper to position
         * @param position Target position
         * @param speedRatio Speed ratio
         * @return true if moved
         */
        bool moveStepperTo(long position, float speedRatio);

        /**
         * @brief Stop the stepper
         * @return true if stopped
         */
        bool stopStepper();

        /**
         * @brief Set error state
         * @param errorCode Error code
         * @param message Error message
         */
        void setError(LiftErrorCode errorCode, const String &message);

        /**
         * @brief Handle initialization sequence
         */
        void initLoop();
    };

} // namespace devices

#endif // COMPOSITION_LIFT_H
