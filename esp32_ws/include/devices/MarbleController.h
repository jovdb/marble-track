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
#include "devices/PowerMonitor.h"
#include "devices/Battery.h"

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
        static constexpr unsigned long WHEEL_SPIN_LONG_PRESS_MS = 500UL;              // Threshold for continuous spin vs short-press breakpoint
        static constexpr unsigned long WHEEL_IDLE_ATTENTION_MS = 3UL * 60UL * 1000UL; // Blink attention after 3 minutes without a button press
        static constexpr int LAUNCHER_WHEEL_BREAKPOINT = 1; ///< Wheel breakpoint index that triggers launcher
        void loopManualLift();
        void loopManualWheel();
        void loopManualSpiral();
        void loopAutoLift();
        void loopAutoWheel();
        void loopAutoSpiral();
        void loopSplitter();
        void loopManualLauncher(bool autoLaunch = false);
        void loopAutoLauncher();
        void loopBattery();

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
        PowerMonitor *_powerMonitor;
        Battery *_battery;

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

        // Launcher
        static constexpr unsigned long LauncherPostLaunchDelayMs = 500UL;
        static constexpr unsigned long LauncherPostLoadDelayMs = 500UL;
        static constexpr unsigned long LauncherAutoInitDelayMs = 2000UL; ///< Delay before auto init starts
        static constexpr float LauncherWheelMinAngle = 32.0f;            ///< Min wheel angle for launch (manual mode)
        static constexpr float LauncherWheelMaxAngle = 58.0f;            ///< Max wheel angle for launch (manual mode)

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
