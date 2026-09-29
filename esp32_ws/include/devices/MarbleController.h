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
#include "devices/WheelLoader.h"
#include "devices/PowerMonitor.h"
#include "devices/Battery.h"

namespace devices
{

    struct TrackLiftState
    {
        uint8_t queueCount = 0;                       // Queued manual lift actions (normally a multiple of 4)
        bool isPressedDuringError = false;            // Lift button pressed (only down) while in error
        bool isTempAutoMode = false;                  // Temporary automatic lift mode
        unsigned long playedBallWaitingSoundTime = 0; // Last ball-waiting sound time
        unsigned long ballReadyWaitingTime = 0;       // Time the ball became ready
        unsigned long autoDelayStartTime = 0;         // Start of the automatic delay
        unsigned long autoDelayMs = 1000;             // Delay between automatic operations
        bool isAutoPowerUnloadPending = false;        // Power unload is pending
        bool isAutoPowerUnloadSongStarted = false;    // Power unload sound started
        unsigned long autoPowerUnloadStartTime = 0;   // Power unload sound start time
        unsigned long autoUpLoadedTime = 0;           // Time lift reached the top loaded
        unsigned long autoNoBallStartTime = 0;        // Start of the no-ball delay
        unsigned long autoNoBallDelayMs = 0;          // Random no-ball delay
        bool isAutoMovingDownSlow = false;            // Lift is moving down slowly
    };

    struct TrackLauncherState
    {
        uint8_t queueCount = 0; // Queued launcher actions
    };

    struct TrackWheelState
    {
        bool isPressedDuringError = false; // Wheel button pressed (only down) while in error
        unsigned long randomDelayMs = 0; // Delay before the next automatic wheel move
    };

    struct TrackSplitterState
    {
        uint8_t queueCount = 0; // Queued splitter sensor pulses
    };
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
        void playButtonCountClick(int count, std::vector<int> additionalReplaceSongIndexes = {});

        int getLiftQueue();

        /**
         * @brief Get the audio device
         * @return Pointer to the audio device
         */
        devices::Hv20tAudio *getAudio()
        {
            return _audio;
        }
        static constexpr unsigned long WHEEL_LONG_PRESS_DURATION_MS = 8000UL;
        static constexpr unsigned long WHEEL_SPIN_LONG_PRESS_MS = 500UL; // Threshold for continuous spin vs short-press breakpoint
        static constexpr int LAUNCHER_WHEEL_BREAKPOINT = 1;              ///< Wheel breakpoint index that triggers launcher
        void loopManualLift();
        void loopManualSpiral();
        void loopAutoLift();
        void loopAutoSpiral();
        void loopSplitter();
        void loopWheel(bool autoMode);
        void loopLauncher(bool autoMode);
        void loopWheelLoader(bool autoMode);
        void loopBattery();
        void loopConfigError();
        void blinkLiftCount();
        void blinkLauncherCount();
        void onWheelStateChange(void *statePtr);
        void onLiftStateChange(void *statePtr);

        Button *_manualButton;
        Buzzer *_buzzer;
        Hv20tAudio *_audio;
        PowerMonitor *_powerMonitor;
        Battery *_battery;

        // Lift
        Lift *_lift;
        Led *_liftLed;
        Button *_liftBtn;
        TrackLiftState _trackLiftState;

        // Wheel
        Wheel *_wheel;
        Led *_wheelLed;
        Button *_wheelBtn;
        TrackWheelState _trackWheelState;

        // Splitter
        Wheel *_splitter;
        TrackSplitterState _trackSplitterState;

        // Spiral
        Led *_spiralLed;
        Button *_spiralBtn;
        Button *_splitterSensor;

        // Launcher
        Launcher *_launcher;
        Led *_launcherLed;
        Button *_launcherBtn;
        TrackLauncherState _trackLauncherState;

        // Wheel Loader
        WheelLoader *_wheelLoader;

        // The delay to wait to notify with blinking led and notification
        const unsigned long _actionNotificationDelayMs = 15000;

        // Launcher
        static constexpr unsigned long LauncherPostLaunchDelayMs = 500UL;
        static constexpr unsigned long LauncherPostLoadDelayMs = 500UL;
        static constexpr unsigned long LauncherAutoInitDelayMs = 2000UL; ///< Delay before auto init starts
        // Todo: make configurable via UI
        static constexpr float LauncherWheelMinAngle = 350.0f; ///< Min wheel angle for launch (manual mode)
        static constexpr float LauncherWheelMaxAngle = 370.0f; ///< Max wheel angle for launch (manual mode)

        // Prevent physical collisions with wheel
        static constexpr float LauncherWheelLoadMinAngle = 180.0f; ///< Min wheel angle for launch (manual mode)
        static constexpr float LauncherWheelLoadMaxAngle = 400.0f; ///< Max wheel angle for launch (manual mode)

        // WheelLoader load ranges
        static constexpr float WheelLoaderRange1Max = 48.0f;
        static constexpr float WheelLoaderRange1Min = WheelLoaderRange1Max - 10.0f;
        static constexpr float WheelLoaderRange2Max = 280.0f;
        static constexpr float WheelLoaderRange2Min = WheelLoaderRange2Max - 10.0f;

        // Idle sound tracking
        unsigned long _lastButtonPressTime = 0;
        bool _idleSoundPlayed = false;

        // Battery
        static constexpr float ShutDownAtPercent = 3; // Shutdown threshold

        bool doPowerShutdown = false;
        bool isAutoMode = false;

    private:
        bool isPlayingOneOfThese(const std::vector<int> &replaceSongIndexes) const;

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
