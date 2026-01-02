/**
 * @file Stepper.h
 * @brief Stepper device using Device with composition mixins
 */

#ifndef COMPOSITION_STEPPER_H
#define COMPOSITION_STEPPER_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/RtosMixin.h"
#include <AccelStepper.h>
#include <freertos/semphr.h>

namespace devices
{

    /**
     * @struct StepperConfig
     * @brief Configuration for Stepper device
     */
    struct StepperConfig
    {
        String name = "Stepper";     // Device name
        String stepperType = "";     // "DRIVER", "HALF4WIRE", "FULL4WIRE"
        float maxSpeed = 1000.0f;    // Maximum speed in steps per second
        float maxAcceleration = 500.0f; // Acceleration in steps per second per second
        float defaultSpeed = 500.0f; // Default speed
        float defaultAcceleration = 250.0f; // Default acceleration
        int stepPin = -1;            // Step pin for DRIVER
        int dirPin = -1;             // Direction pin for DRIVER
        int pin1 = -1;               // Pin 1 for 4-wire
        int pin2 = -1;               // Pin 2 for 4-wire
        int pin3 = -1;               // Pin 3 for 4-wire
        int pin4 = -1;               // Pin 4 for 4-wire
        int enablePin = -1;          // Enable pin
        bool invertEnable = false;   // Invert enable logic
    };

    /**
     * @struct MoveCommand
     * @brief Inter-thread communication for move requests
     */
    struct MoveCommand
    {
        bool pending = false;
        String type = ""; // "move", "moveTo", "stop"
        long steps = 0;
        long position = 0;
        float speed = -1;
        float acceleration = -1;
    };

    /**
     * @struct StepperState
     * @brief State structure for Stepper device
     */
    struct StepperState
    {
        long currentPosition = 0;
        long targetPosition = 0;
        bool isMoving = false;
        bool moveJustStarted = false;
        MoveCommand moveCommand;
    };

    /**
     * @class Stepper
     * @brief Stepper motor control with configurable pins and movement control
     */
    class Stepper : public Device,
                    public ConfigMixin<Stepper, StepperConfig>,
                    public StateMixin<Stepper, StepperState>,
                    public ControllableMixin<Stepper>,
                    public SerializableMixin<Stepper>,
                    public RtosMixin<Stepper>
    {
    public:
        explicit Stepper(const String &id);
        ~Stepper();

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        /**
         * @brief Move the stepper by a number of steps
         * @param steps Number of steps (positive or negative)
         * @param speed Speed in steps/second (-1 for default)
         * @param acceleration Acceleration (-1 for default)
         * @return true if move initiated
         */
        bool move(long steps, float speed = -1, float acceleration = -1);

        /**
         * @brief Move to absolute position
         * @param position Target position
         * @param speed Speed in steps/second (-1 for default)
         * @param acceleration Acceleration (-1 for default)
         * @return true if move initiated
         */
        bool moveTo(long position, float speed = -1, float acceleration = -1);

        /**
         * @brief Stop the stepper
         * @param acceleration Deceleration (-1 for default)
         * @return true if stop initiated
         */
        bool stop(float acceleration = -1);

        /**
         * @brief Set current position
         * @param position New current position
         * @return true if set
         */
        bool setCurrentPosition(long position);

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

        // RTOS task implementation
        void task() override;

        // Plotting
        void plotState() override;

    private:
        AccelStepper *_stepper = nullptr;
        SemaphoreHandle_t _stateMutex;

        void initializeAccelStepper();
        void cleanupAccelStepper();
        void enableStepper();
        void disableStepper();
        void prepareForMove(float &speed, float &acceleration);
        bool ensureReady(const char *action = nullptr, bool logWarning = true) const;
    };

} // namespace devices

#endif // COMPOSITION_STEPPER_H
