#include "track/TrackLift.h"

namespace devices
{
    TrackLiftState::TrackLiftState()
        : queueCount(0),
          isPressedDuringError(false),
          isTempAutoMode(false),
          playedBallWaitingSoundTime(0),
          ballReadyWaitingTime(0),
          autoDelayStartTime(0),
          autoDelayMs(1000),
          isAutoPowerUnloadPending(false),
          isAutoPowerUnloadSongStarted(false),
          autoPowerUnloadStartTime(0),
          autoUpLoadedTime(0),
          autoNoBallStartTime(0),
          autoNoBallDelayMs(0),
          isAutoMovingDownSlow(false),
          previousState(LiftStateEnum::UNKNOWN)
    {
    }
}