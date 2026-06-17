/**
 * @file ServoGate.h
 * @brief ServoGate composite device: button-triggered servo gate with queued pulses
 *
 * On each button click the queue grows by 1 (up to fullQueueCount).
 * While the button is held the queue is set to fullQueueCount.
 * Each queued cycle:  openDelay → servo open → closeDelay → servo close → betweenDelay (if more).
 */

#ifndef COMPOSITION_SERVO_GATE_H
#define COMPOSITION_SERVO_GATE_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/Button.h"
#include "devices/Servo.h"

namespace devices
{

    enum class ServoGateFsmState
    {
        IDLE,
        WAIT_OPEN,
        OPENING,
        WAIT_CLOSE,
        CLOSING,
        BETWEEN
    };

    struct ServoGateConfig
    {
        String name = "ServoGate";
        uint32_t openDelayMs = 500;
        uint32_t closeDelayMs = 1000;
        uint32_t betweenDelayMs = 500;
        int fullQueueCount = 5;
    };

    struct ServoGateState
    {
        String gateState = "Idle";
        int queueCount = 0;
    };

    class ServoGate : public Device,
                      public ConfigMixin<ServoGate, ServoGateConfig>,
                      public StateMixin<ServoGate, ServoGateState>,
                      public ControllableMixin<ServoGate>,
                      public SerializableMixin<ServoGate>
    {
    public:
        explicit ServoGate(const String &id);
        ~ServoGate() = default;

        void setup() override;
        void teardown() override;
        void loop() override;

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        Button *_button = nullptr;
        Servo *_servo = nullptr;

        bool _holdQueueFillApplied = false;

        ServoGateFsmState _fsm = ServoGateFsmState::IDLE;
        unsigned long _timerStart = 0;
        uint32_t _timerDuration = 0;
        bool _childErrorActive = false; // True while a required child is in error

        bool isTimerExpired() const;
        void startTimer(uint32_t durationMs);
        void startCycle();
        String fsmStateToString(ServoGateFsmState state) const;

        /**
         * @brief Check if the gate servo is in error and propagate
         * @return true if the child servo is currently in error
         */
        bool checkChildErrors();
    };

} // namespace devices

#endif // COMPOSITION_SERVO_GATE_H
