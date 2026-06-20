/**
 * @file Launcher.h
 * @brief Launcher composite device: servo arm + ball-waiting button
 *
 * The launcher arm swings up to catch a ball and down to position it for launch.
 * At launch the arm swings up quickly, throwing the ball.
 *
 * States: UNKNOWN → INIT → MOVING_DOWN / MOVING_UP → DOWN / UP
 * Functions: init(), load(), launch()
 */

#ifndef COMPOSITION_LAUNCHER_H
#define COMPOSITION_LAUNCHER_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/Button.h"
#include "devices/Servo.h"

namespace devices
{

    enum class LauncherStateEnum
    {
        UNKNOWN,
        ERROR,
        UP,
        MOVING_UP,
        DOWN,
        MOVING_DOWN
    };

    struct LauncherConfig
    {
        String name = "Launcher";
        long loadTimeMs = 2000; ///< Duration for slow arm movement (load/init)
        long launchTimeMs = 0;  ///< Duration for fast arm movement (launch)
        float loadDistance = 0.8;   ///< How far to move the arm for loading (0.0 = no movement, 1.0 = full range)
    };

    /**
     * @enum WheelErrorCode
     * @brief Enumeration of wheel error codes
     */
    enum class LauncherErrorCode
    {
        None,
        ServoError,
        ButtonError,
        CalibrationZeroNotFound,
        CalibrationSecondZeroNotFound,
    };

    struct LauncherState
    {
        LauncherStateEnum state = LauncherStateEnum::UNKNOWN;
        /** Loading phase */
        uint isLoadingStep = 0;
        /** Launching phase */
        uint isLaunchingStep = 0;
        bool isBallLoaded = false;  ///< Ball is on the arm and ready for launch
        bool isBallWaiting = false; ///< A ball is waiting in the queue (button pressed)
    };

    /**
     * @class Launcher
     * @brief Composite device: servo arm controlled launcher with ball-waiting sensor.
     *
     * Children:
     *  - {id}-servo  : Servo driving the arm (0.0 = DOWN, 1.0 = UP)
     *  - {id}-button : Button detecting a waiting ball in the queue
     */
    class Launcher : public Device,
                     public ConfigMixin<Launcher, LauncherConfig>,
                     public StateMixin<Launcher, LauncherState>,
                     public ControllableMixin<Launcher>,
                     public SerializableMixin<Launcher>
    {
    public:
        explicit Launcher(const String &id);
        ~Launcher() = default;

        void setup() override;
        void teardown() override;
        void loop() override;

        bool init();
        bool load();
        bool launch();

        // ControllableMixin implementation
        void addDeviceStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        Button *_button = nullptr;
        Servo *_servo = nullptr;

        unsigned long _timerStart = 0;
        uint32_t _timerDuration = 0;

        bool isTimerExpired() const;
        void startTimer(uint32_t durationMs);
        void setErrorState(LauncherErrorCode errorCode, const String &errorMessage);
        String errorCodeToString(LauncherErrorCode errorCode) const;

        String stateToString(LauncherStateEnum state) const;

        bool loadLoop();
        bool launchLoop();
        bool moveUp(float amount, long duration);
        bool moveDown();
    };

} // namespace devices

#endif // COMPOSITION_LAUNCHER_H
