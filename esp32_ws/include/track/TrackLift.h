#ifndef TRACKLIFT_H
#define TRACKLIFT_H

#include <Arduino.h>
#include "devices/Lift.h"

namespace devices
{
    struct TrackLiftState
    {
        TrackLiftState();

        static constexpr unsigned long POWER_SONG_DURATION_MS = 5600UL;
        static constexpr unsigned long POWER_SONG_START_DELAY_MS = 500UL;
        static constexpr unsigned long AUTO_POWER_SONG_START_DELAY_MS = 1000UL;
        static constexpr unsigned long AUTO_NO_BALL_RANDOM_MIN_DELAY_MS = 120000UL;
        static constexpr unsigned long AUTO_NO_BALL_RANDOM_MAX_DELAY_MS = 300000UL;
        static constexpr unsigned long ERROR_LONG_PRESS_DURATION_MS = 5000UL;
        static constexpr unsigned long LONG_PRESS_AUTO_MODE_DURATION_MS = 3000UL;
        static constexpr float AUTO_DOWN_NO_BALL_SPEED_RATIO = 0.2f;
        static constexpr float AUTO_DOWN_NORMAL_SPEED_RATIO = 1.0f;
        static constexpr float LIFT_AUTO_SPEED_RATIO = 0.25f;
        static constexpr float LIFT_MANUAL_SPEED_RATIO = 1.0f;
        static constexpr unsigned long BALL_WAITING_NOTIFICATION_FIRST_DELAY_MS = 60000UL;
        static constexpr unsigned long BALL_WAITING_NOTIFICATION_RECURRING_DELAY_MS = 120000UL;
        static constexpr unsigned long LIFT_UP_LOADED_NOTIFICATION_DURATION_MS = 3000UL;
        static constexpr unsigned long LIFT_DOWN_EMPTY_NOTIFICATION_DURATION_MS = 960UL * 5UL;

        uint8_t queueCount;                      // Queued manual lift actions (normally a multiple of 4)
        bool isPressedDuringError;                // Lift button pressed (only down) while in error
        bool isTempAutoMode;                      // Temporary automatic lift mode
        unsigned long playedBallWaitingSoundTime; // Last ball-waiting sound time
        unsigned long ballReadyWaitingTime;       // Time the ball became ready
        unsigned long autoDelayStartTime;         // Start of the automatic delay
        unsigned long autoDelayMs;                // Delay between automatic operations
        bool isAutoPowerUnloadPending;            // Power unload is pending
        bool isAutoPowerUnloadSongStarted;         // Power unload sound started
        unsigned long autoPowerUnloadStartTime;    // Power unload sound start time
        unsigned long autoUpLoadedTime;            // Time lift reached the top loaded
        unsigned long autoNoBallStartTime;         // Start of the no-ball delay
        unsigned long autoNoBallDelayMs;           // Random no-ball delay
        bool isAutoMovingDownSlow;                 // Lift is moving down slowly
        LiftStateEnum previousState;
    };
}

#endif // TRACKLIFT_H