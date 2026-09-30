#include "track/TrackLift.h"
#include "Logging.h"
#include "SongConstants.h"
#include <algorithm>
#include <utility>

namespace devices
{
    TrackLift::TrackLift(Lift &lift, Button &liftButton, Led &liftLed, Hv20tAudio &audio,
                         TrackAudio &trackAudio, TrackLeds &trackLeds)
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
          previousState(LiftStateEnum::UNKNOWN),
          _lift(&lift),
          _liftBtn(&liftButton),
          _liftLed(&liftLed),
          _audio(&audio),
          _trackAudio(trackAudio),
          _trackLeds(trackLeds)
    {
    }

    void TrackLift::setup()
    {
        if (!_unsubscribeLiftStateChange)
        {
            _unsubscribeLiftStateChange = _lift->onStateChange([this](void *statePtr)
                                                               { onStateChange(statePtr, millis()); });
        }
        resetState();
    }

    void TrackLift::teardown()
    {
        if (_unsubscribeLiftStateChange)
        {
            _unsubscribeLiftStateChange();
            _unsubscribeLiftStateChange = {};
        }
        resetState();
    }

    void TrackLift::resetState()
    {
        queueCount = 0;
        isPressedDuringError = false;
        isTempAutoMode = false;
        playedBallWaitingSoundTime = 0;
        ballReadyWaitingTime = 0;
        autoDelayStartTime = 0;
        autoDelayMs = 1000;
        isAutoPowerUnloadPending = false;
        isAutoPowerUnloadSongStarted = false;
        autoPowerUnloadStartTime = 0;
        autoUpLoadedTime = 0;
        autoNoBallStartTime = 0;
        autoNoBallDelayMs = 0;
        isAutoMovingDownSlow = false;
        previousState = LiftStateEnum::UNKNOWN;
    }

    void TrackLift::playLiftError(const String &errorCode)
    {
        if (errorCode == "LIFT_NO_ZERO")
        {
            _trackAudio.playErrorSound(Hv20tPlayMode::QueueIfPlaying, {songs::LIFT_STOP});
            _audio->play(songs::LIFT_NO_ZERO, Hv20tPlayMode::QueueIfPlaying);
        }
        else if (errorCode == "LIFT_INIT_NO_ZERO")
        {
            _trackAudio.playErrorSound(Hv20tPlayMode::QueueIfPlaying, {songs::LIFT_STOP});
            _audio->play(songs::LIFT_INIT_ERROR, Hv20tPlayMode::QueueIfPlaying);
        }
    }
    String TrackLift::toString() const { return "TrackLift"; }

    void TrackLift::loopLed(const LiftState &liftState, unsigned long now)
    {
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            _liftLed->set(true);
            break;
        }
        case devices::LiftStateEnum::ERROR:
        {
            _trackLeds.blinkError(_liftLed);
            break;
        }
        case devices::LiftStateEnum::INIT:
        {
            if (queueCount > 0)
            {
                blinkQueueCount();
            }
            else
            {
                _trackLeds.blinkInit(_liftLed);
            }
            break;
        }
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_DOWN:
        case devices::LiftStateEnum::MOVING_UP:
            if (isAutoMovingDownSlow)
            {
                _liftLed->set(true);
            }
            else if (isTempAutoMode)
            {
                _trackLeds.blinkBusy(_liftLed);
            }
            else if (queueCount > 0)
            {
                blinkQueueCount();
            }
            else
            {
                _liftLed->set(false);
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        {
            if (isTempAutoMode)
            {
                _trackLeds.blinkBusy(_liftLed);
            }
            else if (queueCount > 0)
            {
                blinkQueueCount();
            }
            else
            {
                _liftLed->set(true);
            }
            break;
        }
        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {
            if (ballReadyWaitingTime && !_liftBtn->isPressed() &&
                (ballReadyWaitingTime + TrackLift::BALL_WAITING_NOTIFICATION_FIRST_DELAY_MS) < now &&
                (!playedBallWaitingSoundTime ||
                 (playedBallWaitingSoundTime + TrackLift::BALL_WAITING_NOTIFICATION_RECURRING_DELAY_MS) < now))
            {
                _audio->play(songs::LIFT_BALL_WAITING, devices::Hv20tPlayMode::QueueIfPlaying);
                playedBallWaitingSoundTime = now;
            }

            if (playedBallWaitingSoundTime && playedBallWaitingSoundTime <= now &&
                playedBallWaitingSoundTime + TrackLift::LIFT_UP_LOADED_NOTIFICATION_DURATION_MS > now)
            {
                _trackLeds.blinkAttention(_liftLed);
            }
            else if (isTempAutoMode)
            {
                _trackLeds.blinkBusy(_liftLed);
            }
            else if (queueCount > 0)
            {
                blinkQueueCount();
            }
            else
            {
                _liftLed->set(true);
            }
            break;
        }
        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        {
            if (ballReadyWaitingTime && liftState.ballWaitingSince > 0)
            {
                auto mostRecent = std::max(liftState.ballWaitingSince, ballReadyWaitingTime);
                if ((mostRecent + TrackLift::BALL_WAITING_NOTIFICATION_FIRST_DELAY_MS) < now &&
                    (!playedBallWaitingSoundTime ||
                     (playedBallWaitingSoundTime + TrackLift::BALL_WAITING_NOTIFICATION_RECURRING_DELAY_MS) < now))
                {
                    _audio->play(songs::LIFT_BALL_WAITING, devices::Hv20tPlayMode::QueueIfPlaying);
                    playedBallWaitingSoundTime = now;
                }
            }

            if (playedBallWaitingSoundTime && playedBallWaitingSoundTime <= now &&
                playedBallWaitingSoundTime + TrackLift::LIFT_DOWN_EMPTY_NOTIFICATION_DURATION_MS > now)
            {
                _trackLeds.blinkAttention(_liftLed);
            }
            else if (isTempAutoMode)
            {
                _trackLeds.blinkBusy(_liftLed);
            }
            else if (queueCount > 0)
            {
                blinkQueueCount();
            }
            else
            {
                _liftLed->set(true);
            }
            break;
        }
        }
    }

    void TrackLift::loopManualMode(unsigned long now)
    {
        auto liftState = _lift->getState();
        loopLed(liftState, now);
        // loopLongPress(now);
        loopAutoMode(now);
        loopPowerUnload(now);

        // Lift Logic
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        {
            // Init will start at press
            if (_liftBtn->onPressed())
            {
                _lift->init(TrackLift::LIFT_MANUAL_SPEED_RATIO);
                _trackAudio.playButtonClick();

                // Queue: Load + Move Up
                queueCount += queueCount ? 4 : 2;
            }
            break;
        }

        case devices::LiftStateEnum::ERROR:
        {
            // Pressed
            if (_liftBtn->onPressed())
            {
                isPressedDuringError = true;
            }

            // Short Press
            if (isPressedDuringError && _liftBtn->onShortClick(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
            {
                playLiftError(_lift->getErrorCode());
            }

            // Check for long press while button is held
            if (isPressedDuringError && _liftBtn->onPressedDuration(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting lift init", toString().c_str());
                _lift->init(TrackLift::LIFT_AUTO_SPEED_RATIO);
                _audio->play(songs::LIFT_RESTART, devices::Hv20tPlayMode::StopThenPlay);
            }
            break;
        }
        case devices::LiftStateEnum::INIT:
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                // if only up, go back down, else whole cycle
                queueCount += 4;
                playButtonCountClick();
            }
            break;

        // During actions
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        {
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                // if only up, go back down, else whole cycle
                queueCount += queueCount == 1 ? 6 : 4;
                playButtonCountClick();
            }
            break;
        }
        case devices::LiftStateEnum::MOVING_UP:
        {
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                // if only up, go back down, else whole cycle
                queueCount += queueCount == 0 ? 6 : 4;
                playButtonCountClick();
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                queueCount += 4; // whole cycle
                playButtonCountClick();
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        {

            // Short Press
            if (_liftBtn->onShortClick(TrackLift::LONG_PRESS_AUTO_MODE_DURATION_MS) && queueCount < 240)
            {
                if (!queueCount)
                    _trackAudio.playButtonUp({songs::LIFT_STOP});
                queueCount += queueCount == 0 ? 2 : 4; // to top

                if (_lift->loadBall())
                {
                    queueCount--;
                }
            }

            break;
        }

        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        {
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                // if only up, go back down, else whole cycle
                queueCount += queueCount == 2 ? 6 : 4;
                playButtonCountClick();
            }

            break;
        }

        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {

            if (queueCount == 0)
            {

                // semi long Press, start sound
                if (_liftBtn->onPressedDuration(TrackLift::POWER_SONG_START_DELAY_MS))
                {
                    _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
                }

                // Cancelled long press
                else if (_liftBtn->onShortClick(TrackLift::POWER_SONG_DURATION_MS))
                {
                    _audio->stop();
                    _trackAudio.playButtonUp({songs::LIFT_STOP, songs::LIFT_POWER_UNLOAD});
                }

                // Long Press
                // if (_liftBtn->onPressedDuration(TrackLift::POWER_SONG_DURATION_MS) && queueCount < 240)
                // {
                //     MLOG_INFO("%s: Long press detected (%.2fs), Power unload", toString().c_str());
                //     // Long press: unload with full speed immediately
                //     _lift->unloadBall(0.2f);
                //     queueCount += 1; // unload +  bottom
                // }
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        {
            if (_liftBtn->onPressed() && queueCount < 240)
            {
                queueCount += 4; // whole cycle
                playButtonCountClick();
            }

            break;
        }
        }

        // Process Queue
        loopQueue(liftState);
    }

    bool TrackLift::loopTempAutoMode(unsigned long now)
    {
        // Check if pressed at a valid start
        static auto shouldCheckTempAutoMode = false;
        auto liftState = _lift->getState();
        auto result = false;

        // If on Top, a long press is for power unload
        auto canTempAutoMode = liftState.state != devices::LiftStateEnum::LIFT_UP_LOADED || queueCount > 0;

        // Only allow click from a valid state
        if (liftState.state == devices::LiftStateEnum::ERROR)
        {
            shouldCheckTempAutoMode = false;
        }
        else if (canTempAutoMode && _liftBtn->onPressed())
        {
            shouldCheckTempAutoMode = true;
        }

        // Waiting on a power unload?
        if (canTempAutoMode && shouldCheckTempAutoMode && _liftBtn->onPressedDuration(TrackLift::LONG_PRESS_AUTO_MODE_DURATION_MS))
        {
            if (_lift->isBallWaiting())
            {
                MLOG_INFO("%s: Starting lift auto mode", toString().c_str());

                _audio->play(songs::LIFT_AUTO_MODE_START, devices::Hv20tPlayMode::StopThenPlay);
                isTempAutoMode = true;

                // calc max cycles to prevent overflow
                auto cycles = (255 - queueCount) / 4;
                queueCount += 4 * cycles; // max 60 cycles

                result = true;
            }
            else
            {
                MLOG_DEBUG("%s: Cannot start lift auto mode, no ball waiting", toString().c_str());
                _trackAudio.playErrorSound();
            }
        }

        if (_liftBtn->onReleased())
        {
            shouldCheckTempAutoMode = false;
        }

        return result;
    }

    bool TrackLift::loopPowerUnload(unsigned long now)
    {
        // Check if pressed at a valid start
        static auto shouldCheckPowerUnload = false;
        auto liftState = _lift->getState();
        auto result = false;

        // If on Top, a long press is for power unload
        auto canPowerUnload = liftState.state == devices::LiftStateEnum::LIFT_UP_LOADED && queueCount == 0;

        // Only allow click from a valid state
        if (liftState.state == devices::LiftStateEnum::ERROR)
        {
            shouldCheckPowerUnload = false;
        }
        else if (canPowerUnload && shouldCheckPowerUnload)
        {

            // Semi long Press, start power up sound
            if (_liftBtn->onPressedDuration(TrackLift::POWER_SONG_START_DELAY_MS))
            {
                _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
            }

            // Cancelled long press
            else if (_liftBtn->onShortClick(TrackLift::POWER_SONG_DURATION_MS))
            {
                _audio->stop();
                // _trackAudio.playButtonUp({songs::LIFT_STOP, songs::LIFT_POWER_UNLOAD});
            }

            // Long Press
            if (_liftBtn->onPressedDuration(TrackLift::POWER_SONG_DURATION_MS) && queueCount < 240)
            {
                MLOG_INFO("%s: Long press detected (%.2fs), Power unload", toString().c_str());
                // Long press: unload with full speed immediately
                _lift->unloadBall(0.2f);
                queueCount += 1; // unload +  bottom
            }
        }

        if (_liftBtn->onReleased())
        {
            shouldCheckPowerUnload = false;
        }

        return result;
    }

    bool TrackLift::loopLongPress(unsigned long now)
    {
        // Check if pressed at a valid start
        static auto shouldCheckTempAutoMode = false;
        static auto shouldCheckPowerUnload = false;
        auto liftState = _lift->getState();
        auto result = false;

        auto canPowerUnload = liftState.state == devices::LiftStateEnum::LIFT_UP_LOADED && queueCount == 0;

        // Only allow click from a valid state
        if (liftState.state == devices::LiftStateEnum::ERROR)
        {
            shouldCheckTempAutoMode = false;
            shouldCheckPowerUnload = false;
        }
        else if (_liftBtn->onPressed())
        {
            if (canPowerUnload)
                shouldCheckPowerUnload = true;
            else
                shouldCheckTempAutoMode = true;
        }

        // Waiting on a power unload?
        if (canPowerUnload)
        {
            if (shouldCheckPowerUnload)
            {
                // Go Down, No power unload started yet
                if (_liftBtn->onShortClick(TrackLift::POWER_SONG_DURATION_MS))
                {
                    _trackAudio.playButtonUp({songs::LIFT_STOP, songs::LIFT_POWER_UNLOAD});
                }

                if (_liftBtn->onPressedDuration(TrackLift::POWER_SONG_DURATION_MS))
                {
                    MLOG_INFO("%s: Power unload triggered", toString().c_str());
                    if (_lift->unloadBall(0.2f))
                    {
                        queueCount += 1; // Goto bottom
                    }
                }
            }
        }
        else
        {
            if (shouldCheckTempAutoMode && _liftBtn->onPressedDuration(TrackLift::LONG_PRESS_AUTO_MODE_DURATION_MS))
            {
                if (_lift->isBallWaiting())
                {
                    MLOG_INFO("%s: Starting lift auto mode", toString().c_str());

                    _audio->play(songs::LIFT_AUTO_MODE_START, devices::Hv20tPlayMode::StopThenPlay);
                    isTempAutoMode = true;

                    // calc max cycles to prevent overflow
                    auto cycles = (255 - queueCount) / 4;
                    queueCount += 4 * cycles; // max 60 cycles

                    result = true;
                }
                else
                {
                    MLOG_DEBUG("%s: Cannot start lift auto mode, no ball waiting", toString().c_str());
                    _trackAudio.playErrorSound();
                }
            }
        }

        if (_liftBtn->onReleased())
        {
            shouldCheckTempAutoMode = false;
            shouldCheckPowerUnload = false;
        }

        return result;
    }

    bool TrackLift::loopShortPressQueue(unsigned long now)
    {
        static auto shouldCheckShortPress = false;
        auto liftState = _lift->getState();
        auto result = false;

        // Only allow click from a valid state
        if (liftState.state == devices::LiftStateEnum::ERROR)
            shouldCheckShortPress = false;
        else if (_liftBtn->onPressed())
            shouldCheckShortPress = true;

        // Check click
        switch (liftState.state)
        {

        case devices::LiftStateEnum::ERROR:
        {
            // No short click supported
            break;
        }
        case devices::LiftStateEnum::INIT:

        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        {
            if (!shouldCheckShortPress)
                return result;

            if (queueCount >= 240)
                return result;

            // Down: Play sound
            if (!queueCount)
            {
                if (_liftBtn->onPressed())
                    _trackAudio.playButtonDown({songs::LIFT_STOP});
            }

            // Action
            if (!queueCount
                    ? _liftBtn->onShortClick(TrackLift::LONG_PRESS_AUTO_MODE_DURATION_MS)
                    : _liftBtn->onPressed()) // Immediate
            {

                if (!queueCount)
                    _trackAudio.playButtonUp({songs::LIFT_STOP});
                else
                    playButtonCountClick();

                // if only up, go back down, else whole cycle
                queueCount += queueCount ? 2 : 4;
            }
        }
        break;
        }

        if (_liftBtn->onReleased())
            shouldCheckShortPress = false;

        return result;
    }

    void TrackLift::loopAutoMode(unsigned long now)
    {
        // Auto lift control logic - automatic cycling through lift operations
        auto liftState = _lift->getState();

        if (liftState.state != devices::LiftStateEnum::LIFT_UP_LOADED)
        {
            isAutoPowerUnloadPending = false;
            isAutoPowerUnloadSongStarted = false;
            autoPowerUnloadStartTime = 0;
            autoUpLoadedTime = 0;
        }

        if (liftState.state != devices::LiftStateEnum::LIFT_DOWN_EMPTY || liftState.ballWaitingSince > 0)
        {
            autoNoBallStartTime = 0;
            autoNoBallDelayMs = 0;
        }

        if (liftState.state != devices::LiftStateEnum::MOVING_DOWN)
        {
            isAutoMovingDownSlow = false;
        }

        // LED
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            _liftLed->set(false);
            break;
        case devices::LiftStateEnum::ERROR:
            _trackLeds.blinkError(_liftLed);
            break;
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        case devices::LiftStateEnum::LIFT_UP_LOADED:
            _trackLeds.blinkBusy(_liftLed);
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            if (isAutoMovingDownSlow)
            {
                _liftLed->set(true);
            }
            else
            {
                _trackLeds.blinkBusy(_liftLed);
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        {
            _liftLed->set(true);
            break;
        }
        }

        // LOGIC
        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            _lift->init(TrackLift::LIFT_AUTO_SPEED_RATIO);
            break;

        case devices::LiftStateEnum::ERROR:

            // Pressed
            if (_liftBtn->onPressed())
            {
                isPressedDuringError = true;
            }

            // Short Press
            if (isPressedDuringError && _liftBtn->onShortClick(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
            {
                playLiftError(_lift->getErrorCode());
            }

            // Check for long press while button is held
            if (isPressedDuringError && _liftBtn->onPressedDuration(TrackLift::ERROR_LONG_PRESS_DURATION_MS))
            {
                MLOG_INFO("%s: Error recovery long press detected in auto mode, starting lift init", toString().c_str());
                _lift->init(TrackLift::LIFT_AUTO_SPEED_RATIO);
                _audio->play(songs::LIFT_RESTART, devices::Hv20tPlayMode::StopThenPlay);
            }
            break;

        // BUSY states - just blink LED
        case devices::LiftStateEnum::INIT:
            if (_liftBtn->onPressed())
            {
                _trackAudio.playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                _audio->play(songs::LIFT_INIT_BUSY, devices::Hv20tPlayMode::QueueIfPlaying);
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
            if (_liftBtn->onPressed())
                _trackAudio.playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
            break;

        case devices::LiftStateEnum::MOVING_DOWN:

            if (isAutoMovingDownSlow && liftState.ballWaitingSince > 0)
            {
                if (_lift->down(TrackLift::AUTO_DOWN_NORMAL_SPEED_RATIO * TrackLift::LIFT_AUTO_SPEED_RATIO))
                {
                    isAutoMovingDownSlow = false;
                    MLOG_INFO("%s: Ball waiting detected during auto down, switching to normal speed", toString().c_str());
                }
            }
            else if (_liftBtn->onPressed())
            {
                if (isAutoMovingDownSlow)
                {
                    if (_lift->down(TrackLift::AUTO_DOWN_NORMAL_SPEED_RATIO * TrackLift::LIFT_AUTO_SPEED_RATIO))
                    {
                        isAutoMovingDownSlow = false;
                        _trackAudio.playButtonClick({songs::LIFT_STOP});
                    }
                }
                else
                {
                    _trackAudio.playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});
                }
            }

            break;

        case devices::LiftStateEnum::LIFT_DOWN_EMPTY:
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
        {
            if (liftState.state == devices::LiftStateEnum::LIFT_DOWN_LOADED)
            {
                // Loaded: move up to unload position
                _lift->up(TrackLift::LIFT_AUTO_SPEED_RATIO);
                autoDelayStartTime = 0; // Reset delay timer
            }
            else if (liftState.ballWaitingSince > 0)
            {
                autoNoBallStartTime = 0;
                autoNoBallDelayMs = 0;

                // Not loaded: wait 1000ms before starting load
                if (autoDelayStartTime == 0)
                {
                    autoDelayStartTime = now;
                    break;
                }

                if ((now - autoDelayStartTime) < autoDelayMs)
                {
                    break;
                }

                _lift->loadBall();
                autoDelayStartTime = 0;
            }
            else
            {
                autoDelayStartTime = 0;

                if (_liftBtn->onPressed())
                {
                    _trackAudio.playButtonDown({songs::LIFT_STOP});
                }

                if (_liftBtn->onReleased())
                {
                    _trackAudio.playButtonUp({songs::LIFT_STOP});
                    autoNoBallStartTime = 0;
                    autoNoBallDelayMs = 0;
                    _lift->loadBall();
                    break;
                }

                if (autoNoBallStartTime == 0)
                {
                    autoNoBallStartTime = now;
                    autoNoBallDelayMs = random(
                        TrackLift::AUTO_NO_BALL_RANDOM_MIN_DELAY_MS,
                        TrackLift::AUTO_NO_BALL_RANDOM_MAX_DELAY_MS + 1UL);
                    break;
                }

                if ((now - autoNoBallStartTime) >= autoNoBallDelayMs)
                {
                    MLOG_INFO("%s: Auto lift random start (no ball waiting) after %lus",
                              toString().c_str(),
                              autoNoBallDelayMs / 1000UL);

                    if (_lift->loadBall())
                    {
                        autoNoBallStartTime = 0;
                        autoNoBallDelayMs = 0;
                    }
                }
            }
            break;
        }

        case devices::LiftStateEnum::LIFT_UP_EMPTY:
        case devices::LiftStateEnum::LIFT_UP_LOADED:
        {
            if (_liftBtn->onPressed())
                _trackAudio.playErrorSound(devices::Hv20tPlayMode::SkipIfPlaying, {songs::LIFT_STOP});

            // Check if we need to wait before next operation
            if (autoDelayStartTime > 0 && (now - autoDelayStartTime) < autoDelayMs)
            {
                // Still waiting, do nothing
                break;
            }

            if (liftState.state == devices::LiftStateEnum::LIFT_UP_LOADED)
            {
                if (autoUpLoadedTime == 0)
                {
                    autoUpLoadedTime = now;
                    isAutoPowerUnloadPending = (random(100) < 25);
                    isAutoPowerUnloadSongStarted = false;
                    autoPowerUnloadStartTime = 0;
                    break;
                }

                const unsigned long loadedLiftUpElapsed = now - autoUpLoadedTime;

                // Wait until lift-end song is ready (loaded LIFT_UP + 1000ms)
                if (loadedLiftUpElapsed < TrackLift::AUTO_POWER_SONG_START_DELAY_MS)
                {
                    break;
                }

                if (isAutoPowerUnloadPending)
                {
                    if (!isAutoPowerUnloadSongStarted)
                    {
                        _audio->play(songs::LIFT_POWER_UNLOAD, devices::Hv20tPlayMode::StopThenPlay);
                        isAutoPowerUnloadSongStarted = true;
                        autoPowerUnloadStartTime = now;
                        break;
                    }

                    const unsigned long powerSongElapsed = now - autoPowerUnloadStartTime;
                    if (powerSongElapsed >= TrackLift::POWER_SONG_DURATION_MS - 500)
                    {
                        if (_lift->unloadBall(0.2f))
                        {
                            isAutoPowerUnloadPending = false;
                            isAutoPowerUnloadSongStarted = false;
                            autoPowerUnloadStartTime = 0;
                            autoUpLoadedTime = 0;
                            autoDelayStartTime = 0;
                        }
                    }
                }
                else
                {
                    // Non power unload: also wait 1000ms at loaded LIFT_UP, then unload normally
                    if (_lift->unloadBall(1.0f))
                    {
                        autoUpLoadedTime = 0;
                        autoDelayStartTime = 0;
                    }
                }
            }
            else
            {
                // Not loaded: move down to loading position
                if (liftState.ballWaitingSince > 0)
                {
                    isAutoMovingDownSlow = false;
                    _lift->down(TrackLift::AUTO_DOWN_NORMAL_SPEED_RATIO * TrackLift::LIFT_AUTO_SPEED_RATIO);
                }
                else
                {
                    if (_lift->down(TrackLift::AUTO_DOWN_NO_BALL_SPEED_RATIO * TrackLift::LIFT_AUTO_SPEED_RATIO))
                    {
                        isAutoMovingDownSlow = true;
                    }
                }
                autoDelayStartTime = 0; // Reset delay timer
                isAutoPowerUnloadPending = false;
                isAutoPowerUnloadSongStarted = false;
                autoPowerUnloadStartTime = 0;
                autoUpLoadedTime = 0;
            }
            break;
        }
        }
    }

    int TrackLift::getQueue() const
    {
        int offset = 0;
        switch (_lift->getState().state)
        {
        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::ERROR:
        case LiftStateEnum::INIT:
            break;
        case LiftStateEnum::LIFT_DOWN_EMPTY:
            offset = -4;
            break;
        case LiftStateEnum::LIFT_DOWN_LOADING:
        case LiftStateEnum::LIFT_DOWN_LOADED:
            offset = -3;
            break;
        case LiftStateEnum::MOVING_UP:
        case LiftStateEnum::LIFT_UP_LOADED:
            offset = -2;
            break;
        case LiftStateEnum::LIFT_UP_UNLOADING:
        case LiftStateEnum::LIFT_UP_EMPTY:
            offset = -1;
            break;
        case LiftStateEnum::MOVING_DOWN:
            break;
        }

        return (queueCount + offset) / 4 + 1;
    }

    void TrackLift::playButtonCountClick()
    {
        auto count = getQueue();
        _trackAudio.playButtonCountClick(count, {songs::LIFT_STOP});
    }

    void TrackLift::loopQueue(const LiftState &liftState)
    {
        if (queueCount == 0)
            return;

        switch (liftState.state)
        {

        case LiftStateEnum::UNKNOWN:
        case LiftStateEnum::ERROR:
            queueCount = 0;
            break;
        case LiftStateEnum::INIT:
            break;
        case LiftStateEnum::LIFT_DOWN_EMPTY:
            if (_lift->isBallWaiting())
            {
                if (_lift->loadBall())
                {
                    queueCount--;
                }
            }
            else if (isTempAutoMode)
            {
                queueCount = 0;
                isTempAutoMode = false;
                if (_audio->getPlayingIndex() == songs::LIFT_STOP)
                {
                    _audio->play(songs::LIFT_AUTO_MODE_END, devices::Hv20tPlayMode::StopThenPlay);
                }
                else
                {
                    _audio->play(songs::LIFT_AUTO_MODE_END, devices::Hv20tPlayMode::QueueIfPlaying);
                }
            }
            break;
        case LiftStateEnum::LIFT_DOWN_LOADED:
            if (_lift->up(TrackLift::LIFT_MANUAL_SPEED_RATIO))
            {
                queueCount--;
            }
            break;
        case LiftStateEnum::LIFT_UP_LOADED:
            if (_lift->unloadBall(1.0f))
            {
                queueCount--;
            }
            break;
        case LiftStateEnum::LIFT_UP_EMPTY:
            if (_lift->down(TrackLift::LIFT_MANUAL_SPEED_RATIO))
            {
                queueCount--;
            }
            break;
        default:
            break;
        }
    }

    void TrackLift::blinkQueueCount()
    {
        int queued = queueCount > 0 ? getQueue() : 1;
        _trackLeds.blinkCount(_liftLed, queued);
    }

    void TrackLift::onStateChange(void *statePtr, unsigned long now)
    {
        auto *liftState = static_cast<LiftState *>(statePtr);
        if (!liftState)
        {
            return;
        }

        if (previousState != LiftStateEnum::LIFT_UP_LOADED && liftState->state == LiftStateEnum::LIFT_UP_LOADED)
        {
            ballReadyWaitingTime = now;
            _audio->play(songs::LIFT_STOP, Hv20tPlayMode::SkipIfPlaying);
        }

        if (previousState != LiftStateEnum::LIFT_DOWN_EMPTY && liftState->state == LiftStateEnum::LIFT_DOWN_EMPTY)
        {
            ballReadyWaitingTime = now;
            _audio->play(songs::LIFT_STOP, Hv20tPlayMode::QueueIfPlaying);
        }

        if (previousState == LiftStateEnum::LIFT_DOWN_EMPTY && liftState->state != LiftStateEnum::LIFT_DOWN_EMPTY)
        {
            playedBallWaitingSoundTime = 0;
        }

        if (previousState == LiftStateEnum::ERROR && liftState->state != LiftStateEnum::ERROR)
        {
            isPressedDuringError = false;
            _audio->removeFromQueue(songs::LIFT_NO_ZERO);
        }

        if (previousState != LiftStateEnum::ERROR && liftState->state == LiftStateEnum::ERROR)
        {
            playLiftError(_lift->getErrorCode());
        }

        previousState = liftState->state;
    }
}