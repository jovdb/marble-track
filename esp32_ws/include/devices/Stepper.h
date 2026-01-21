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
#include "pins/IPin.h"
#include "pins/Pins.h"
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
        PinConfig stepPin;           // Step pin for DRIVER
        PinConfig dirPin;            // Direction pin for DRIVER
        PinConfig pin1;              // Pin 1 for 4-wire
        PinConfig pin2;              // Pin 2 for 4-wire
        PinConfig pin3;              // Pin 3 for 4-wire
        PinConfig pin4;              // Pin 4 for 4-wire
        PinConfig enablePin;         // Enable pin
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
        std::vector<String> getPins() const override;

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

    private:
        AccelStepper *_driver = nullptr;
        SemaphoreHandle_t _stateMutex;

        pins::IPin *_stepPin = nullptr;
        pins::IPin *_dirPin = nullptr;
        pins::IPin *_pin1 = nullptr;
        pins::IPin *_pin2 = nullptr;
        pins::IPin *_pin3 = nullptr;
        pins::IPin *_pin4 = nullptr;
        pins::IPin *_enablePin = nullptr;

        void initializeAccelStepper();
        void cleanupAccelStepper();
        void cleanupPins();
        void enableStepper();
        void disableStepper();
        void prepareForMove(float &speed, float &acceleration);
        bool ensureReady(const char *action = nullptr, bool logWarning = true) const;
    };

} // namespace devices

#endif // COMPOSITION_STEPPER_H
