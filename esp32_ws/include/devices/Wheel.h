/**
 * @file Wheel.h
 * @brief Wheel device using Device with composition mixins
 */

#ifndef COMPOSITION_WHEEL_H
#define COMPOSITION_WHEEL_H

#include "devices/Device.h"
#include "devices/Stepper.h"
#include "devices/Button.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include <freertos/semphr.h>

namespace devices
{

    /**
     * @enum WheelStateEnum
     * @brief Enumeration of possible wheel states
     */
    enum class WheelStateEnum
    {
        UNKNOWN,
        CALIBRATING,
        IDLE,
        MOVING,
        INIT,
        ERROR,
    };

    /**
     * @struct WheelConfig
     * @brief Configuration for Wheel device
     */
    struct WheelConfig
    {
        String name = "Wheel";              // Device name
        long stepsPerRevolution = 0;        // Steps per full revolution
        long maxStepsPerRevolution = 10000; // Maximum steps for calibration
        float zeroPointDegree = 0.0f;       // Zero point offset in degrees
        std::vector<float> breakPoints;     // Breakpoint angles in degrees
        int direction = 1;                  // Rotation direction (-1 = CCW, 1 = CW)
    };

    /**
     * @struct WheelState
     * @brief State structure for Wheel device
     */
    struct WheelState
    {
        WheelStateEnum state = WheelStateEnum::UNKNOWN; // Current wheel state
        long lastZeroPosition = 0;                      // Position at last zero sensor trigger
        long stepsInLastRevolution = 0;                 // Steps measured in last revolution
        int currentBreakpointIndex = -1;                // Current breakpoint index
        int targetBreakpointIndex = -1;                 // Target breakpoint index
        float targetAngle = -1.0f;                      // Target angle for current movement
        bool onError = false;                           // Error flag
        bool breakpointChanged = false;                 // Flag for breakpoint index change
        bool zeroSensorWasPressed = false;              // Previous zero sensor state for edge detection
    };

    /**
     * @class Wheel
     * @brief Wheel device with stepper motor and sensor control
     *
     * Manages a wheel with stepper motor positioning, zero sensor, and breakpoint navigation.
     * Uses composition pattern with children devices (Stepper, Button sensors).
     */
    class Wheel : public Device,
                  public ConfigMixin<Wheel, WheelConfig>,
                  public StateMixin<Wheel, WheelState>,
                  public ControllableMixin<Wheel>,
                  public SerializableMixin<Wheel>
    {
    public:
        explicit Wheel(const String &id);
        ~Wheel();

        void setup() override;
        void teardown() override;
        void loop() override;

        /**
         * @brief Move the wheel by a specified number of steps
         * @param steps Number of steps to move
         * @return true if move initiated, false otherwise
         */
        bool move(long steps);

        /**
         * @brief Calibrate the wheel by measuring steps per revolution
         * @return true if calibration started, false otherwise
         */
        bool calibrate();

        /**
         * @brief Initialize the wheel to find zero position
         * @return true if init started, false otherwise
         */
        bool init();

        /**
         * @brief Move to a specific angle (0-359.9 degrees)
         * @param angle Target angle in degrees
         * @return true if move initiated, false otherwise
         */
        bool moveToAngle(float angle);

        /**
         * @brief Move to the next breakpoint
         * @return true if move initiated, false otherwise
         */
        bool nextBreakPoint();

        /**
         * @brief Get current breakpoint index
         * @return Current breakpoint index (-1 if not set)
         */
        int getCurrentBreakpointIndex() const;

        /**
         * @brief Stop the wheel movement
         * @return true if stopped, false otherwise
         */
        bool stop();

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    protected:
        Stepper *_stepper;   // Stepper motor child device
        Button *_zeroSensor; // Zero sensor child device

    private:
        /**
         * @brief Convert WheelStateEnum to string
         * @param state State enum value
         * @return String representation
         */
        String stateToString(WheelStateEnum state) const;

        /**
         * @brief Notify clients about steps per revolution measurement
         * @param steps Number of steps measured
         */
        void notifyStepsPerRevolution(long steps);

        unsigned long _initStartTime = 0; // Start time for init operation
        bool _waitingForMoveStart = false;
        bool _moveHasStarted = false;
    };

} // namespace devices

#endif // COMPOSITION_WHEEL_H
