#ifndef MARBLECONTROLLER_H
#define MARBLECONTROLLER_H

#include <Arduino.h>
#include <functional>
#include <vector>
#include "Device.h"
#include "devices/Button.h"
#include "devices/Wheel.h"
#include "devices/Buzzer.h"
#include "devices/Hv20tAudio.h"
#include "devices/Led.h"
#include "devices/Lift.h"
#include "devices/Launcher.h"

namespace devices
{

    class MarbleController : public Device
    {
    public:
        MarbleController(const String &id);
        void setup() override;
        void teardown() override;
        void loop() override;

        /**
         * @brief Play an error sound using the buzzer
         */
        void playErrorSound(Hv20tPlayMode mode = Hv20tPlayMode::SkipIfPlaying, std::vector<int> additionalReplaceSongIndexes = {});

        /**
         * @brief Play a click sound using the buzzer
         */
        void playClickSound();
        void playClickOffSound();
        void playStartupSound();
        void playButtonDown(std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonUp(std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonClick(std::vector<int> additionalReplaceSongIndexes = {});

        /**
         * @brief Get the audio device
         * @return Pointer to the audio device
         */
        devices::Hv20tAudio *getAudio()
        {
            return _audio;
        }
        static constexpr unsigned long WHEEL_LONG_PRESS_DURATION_MS = 8000UL;
        static constexpr int LAUNCHER_WHEEL_BREAKPOINT = 0; ///< Wheel breakpoint index that triggers launcher
        void loopManualLift();
        void loopManualWheel();
        void loopManualSpiral();
        void loopAutoLift();
        void loopAutoWheel();
        void loopAutoSpiral();
        void loopSplitter();
        void loopManualLauncher();
        void loopAutoLauncher();
        void blinkError(Led *ledDevice);
        void blinkBusy(Led *ledDevice);
        void blinkInit(Led *ledDevice);
        void blinkAttention(Led *ledDevice);
        void onWheelStateChange(void *statePtr);
        void onLiftStateChange(void *statePtr);
        Button *_manualButton;
        Buzzer *_buzzer;
        Hv20tAudio *_audio;
        Lift *_lift;
        Wheel *_wheel;
        Wheel *_splitter;
        Led *_liftLed;
        Button *_liftBtn;
        Led *_wheelLed;
        Button *_wheelBtn;
        Led *_spiralLed;
        Button *_spiralBtn;
        Button *_splitterSensor;
        Launcher *_launcher;
        Led *_launcherLed;
        Button *_launcherBtn;

        // Splitter sensor pulse counter and delay logic
        uint8_t _splitterCounter = 0;
        unsigned long _splitterDelayStart = 0;
        unsigned long _splitterSensorPressStartTime = 0;
        bool _splitterSensorWasPressed = false;
        bool _splitterLongPressApplied = false;
        bool _splitterMovePending = false;
        bool _splitterMoveSawBusy = false;

        // Button timing for unload duration control
        unsigned long _liftButtonPressStartTime = 0;
        bool _isBallStillLoaded = false;
        bool _isLiftPowerUnloadSongPlaying = false;
        uint8_t _liftQueuedPresses = 0;

        // Wheel button long press tracking
        inline static unsigned long _wheelButtonPressStartTime = 0;
        inline static bool _wheelButtonLongPressTriggered = false;

        // Auto lift timing control
        unsigned long _autoLiftDelayStart = 0;
        unsigned long _autoLiftDelayMs = 1000; // 1 second delay between auto operations
        bool _autoPowerUnloadPending = false;
        bool _autoPowerUnloadSongStarted = false;
        unsigned long _autoPowerUnloadStartTime = 0;
        unsigned long _autoLiftUpLoadedSince = 0;
        unsigned long _autoNoBallLiftStartTime = 0;
        unsigned long _autoNoBallLiftDelayMs = 0;
        bool _autoLiftMovingDownSlow = false;

        // Launcher manual mode timing
        enum class LauncherPhase
        {
            IDLE,
            WAITING_FOR_INIT,  ///< Waiting before triggering launcher init (auto: timer, manual: button press)
            POST_LAUNCH_DELAY, ///< Waiting 500 ms after launch before calling load()
            LOADING,           ///< load() or init() called; waiting for arm to finish moving
            POST_LOAD_DELAY    ///< Waiting 500 ms after load before accepting next press
        };
        LauncherPhase _launcherPhase = LauncherPhase::IDLE;
        unsigned long _launcherPhaseStart = 0;
        static constexpr unsigned long LauncherPostLaunchDelayMs = 500UL;
        static constexpr unsigned long LauncherPostLoadDelayMs = 500UL;
        static constexpr unsigned long LauncherAutoInitDelayMs = 2000UL; ///< Delay before auto init starts
        static constexpr float LauncherWheelMinAngle = 30.0f;           ///< Min wheel angle for launch (manual mode)
        static constexpr float LauncherWheelMaxAngle = 60.0f;           ///< Max wheel angle for launch (manual mode)

        // Launcher auto mode
        uint8_t _autoLauncherBallsToLaunch = 0; ///< Number of remaining launches in current auto sequence
        LauncherPhase _autoLauncherPhase = LauncherPhase::IDLE;
        unsigned long _autoLauncherPhaseStart = 0;

        // Launcher manual mode – launch count limit per wheel range entry
        static constexpr uint8_t LauncherMaxLaunchesPerRangeEntry = 2;
        uint8_t _manualLauncherLaunchCount = 0; ///< Launches fired since wheel entered allowed range
        bool _manualWheelWasInRange = false;     ///< Whether wheel was in range last loop

        // 0 = not idle, >0 = idle start time
        unsigned long _wheelIdleStartTime = 0;
        // Random delay before next wheel trigger
        unsigned long _randomWheelDelayMs = 0;

        // Idle sound tracking
        unsigned long _lastButtonPressTime = 0;
        bool _idleSoundPlayed = false;

        bool isAutoMode = false;

    private:
        /**
         * @brief Play lift-specific error sounds based on error code
         * @param liftState Pointer to the lift state containing error information
         */
        void playLiftError(const String &errorCode);

        /**
         * @brief Play wheel-specific error sounds based on error code
         * @param wheelState Pointer to the wheel state containing error information
         */
        void playWheelError(const String &errorCode);
    };

} // namespace devices

#endif // MARBLECONTROLLER_H
