#include "AutoMode.h"
#include "Logging.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"
#include "devices/Lift.h"

AutoMode::AutoMode(DeviceManager &deviceManager) : deviceManager(deviceManager)
{
    _buzzer = nullptr;

    _wheel = nullptr;
    _wheelBtnLed = nullptr;

    _splitter = nullptr;
    _splitterBtnLed = nullptr;

    _lift = nullptr;
}

void AutoMode::setup()
{
    // TODO: Re-enable when legacy devices are converted to composition devices
    // _wheel = deviceManager.getDeviceByIdAs<devices::Wheel>("wheel");
    // if (_wheel == nullptr)
    // {
    //     MLOG_ERROR("Required device 'wheel' not found!");
    // }

    // _buzzer = deviceManager.getDeviceByIdAs<devices::Buzzer>("buzzer");
    // if (_buzzer == nullptr)
    // {
    //     MLOG_ERROR("Required device 'buzzer' not found!");
    // }

    // _wheelBtnLed = deviceManager.getDeviceByIdAs<devices::Led>("wheel-btn-led");
    // if (_wheelBtnLed == nullptr)
    // {
    //     MLOG_ERROR("Required device 'wheel-btn-led' not found!");
    // }

    // _splitter = deviceManager.getDeviceByIdAs<devices::Wheel>("splitter");
    // if (_splitter == nullptr)
    // {
    //     MLOG_ERROR("Required device 'splitter' not found!");
    // }

    // _splitterBtnLed = deviceManager.getDeviceByIdAs<devices::Led>("splitter-btn-led");
    // if (_splitterBtnLed == nullptr)
    // {
    //     MLOG_ERROR("Required device 'splitter-btn-led' not found!");
    // }

    // _lift = deviceManager.getDeviceByIdAs<devices::Lift>("lift");
    // if (_lift == nullptr)
    // {
    //     MLOG_ERROR("Required device 'lift' not found!");
    // }

    MLOG_INFO("AutoMode setup complete (disabled - waiting for composition devices)");
}

void AutoMode::loop()
{
    const ulong currentMillis = millis();

    // Reset devices at start
    if (_wheel && _wheel->getState().state == devices::WheelStateEnum::UNKNOWN)
    {
        MLOG_INFO("Initializing Wheel");
        _wheel->init();
    }

    static ulong liftInitStartTime = 0;
    if (_lift && _lift->getState().state == devices::LiftStateEnum::UNKNOWN)
    {
        if (liftInitStartTime == 0)
        {
            liftInitStartTime = currentMillis;
        }
        else if (currentMillis >= liftInitStartTime + 3000)
        {
            MLOG_INFO("Initializing Lift");
            _lift->init();
            liftInitStartTime = 0;
        }
    }

    static ulong splitterInitStartTime = 0;
    if (_splitter && _splitter->getState().state == devices::WheelStateEnum::UNKNOWN)
    {
        if (splitterInitStartTime == 0)
        {
            splitterInitStartTime = currentMillis;
        }
        else if (currentMillis >= splitterInitStartTime + 7000)
        {
            MLOG_INFO("Initializing Splitter");
            _splitter->init();
            splitterInitStartTime = 0;
        }
    }

    const ulong currentSec = round(currentMillis / 1000);

    // Loop button leds
    const byte stepCount = 3;
    const int stepDurationMs = 200;

    int step = static_cast<int>(std::floor((currentMillis % (stepCount * stepDurationMs)) / stepDurationMs));
    if (_wheelBtnLed)
        _wheelBtnLed->set(step == 1);
    if (_splitterBtnLed)
        _splitterBtnLed->set(step == 2);

    // Wheel ball exit counting
    static uint wheelExitMaxBallCount = 2; // Max number of ball loaded due to reset
    static ulong lastWheelExitTime = 0;
    if (_wheel && _wheel->getState().breakpointChanged)
    {
        // Increase on Ball Exit breakpoints
        const int currentIndex = _wheel->getCurrentBreakpointIndex();
        if (currentIndex == 1 || currentIndex == 2)
        {
            MLOG_INFO("AutoMode: Possible ball unloaded from wheel");
            wheelExitMaxBallCount++;
            lastWheelExitTime = currentMillis;
        }
    }
    /*
        const bool allInitialized =
            (_wheel && _wheel->getState().state != devices::WheelStateEnum::UNKNOWN) &&
            (_splitter && _splitter->getState().state != devices::WheelStateEnum::UNKNOWN) &&
            (_lift && _lift->getState().state != devices::LiftStateEnum::UNKNOWN);

        if (!allInitialized)
            return;
    */
    // Wheel
    static ulong wheelIdleStartTime = 0;
    static int randomWheelDelayMs = 0;
    static bool wheelDelaySet = false;

    if (_wheel)
        switch (_wheel->getState().state)
        {
        case devices::WheelStateEnum::UNKNOWN:
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::ERROR:
            wheelDelaySet = false;
            break;
        case devices::WheelStateEnum::MOVING:
            wheelDelaySet = false;
            break;
        case devices::WheelStateEnum::IDLE:
            if (!wheelDelaySet)
            {
                wheelIdleStartTime = currentMillis;
                randomWheelDelayMs = random(100, 20000);
                wheelDelaySet = true;
                MLOG_INFO("AutoMode: Next wheel trigger in %d ms", randomWheelDelayMs);
            }
            else if (currentMillis >= wheelIdleStartTime + randomWheelDelayMs)
            {
                MLOG_INFO("AutoMode: Triggering wheel next breakpoint");
                _wheel->nextBreakPoint();
                wheelDelaySet = false;
            }
            break;
        default:
            MLOG_WARN("AutoMode: Unknown wheel state");
            break;
        }

    // Splitter
    if (_splitter)
        switch (_splitter->getState().state)
        {
        case devices::WheelStateEnum::UNKNOWN:
        case devices::WheelStateEnum::CALIBRATING:
        case devices::WheelStateEnum::INIT:
        case devices::WheelStateEnum::MOVING:
        case devices::WheelStateEnum::ERROR:
            break;
        case devices::WheelStateEnum::IDLE:
            if (wheelExitMaxBallCount > 0)
            {

                if (lastWheelExitTime > 0 && currentMillis > lastWheelExitTime + 500)
                {
                    wheelExitMaxBallCount--;
                    MLOG_INFO("AutoMode: Splitter turns because wheel ball unloading");
                    lastWheelExitTime = 0;
                    _splitter->nextBreakPoint();
                }
                else
                {
                    // Possible balls
                    static ulong splitterIdleStartTime = 0;
                    static int randomSplitterDelayMs = 0;
                    static bool splitterDelaySet = false;

                    if (!splitterDelaySet)
                    {
                        splitterIdleStartTime = currentMillis;
                        randomSplitterDelayMs = random(30000, 120000); // 60s to 300s
                        splitterDelaySet = true;
                    }
                    else if (currentMillis >= splitterIdleStartTime + randomSplitterDelayMs)
                    {
                        wheelExitMaxBallCount--;
                        MLOG_INFO("AutoMode: Splitter turns to unload possible balls, remaining possible balls: %d", wheelExitMaxBallCount);
                        _splitter->nextBreakPoint();
                        splitterDelaySet = false;
                    }
                }
            }
            break;
        default:
            MLOG_WARN("AutoMode: Unknown splitter state");
            break;
        }

    // Lift
    static bool _speedMoveDownCalled = false;
    if (_lift)
        switch (_lift->getState().state)
        {
        case devices::LiftStateEnum::UNKNOWN:
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::ERROR:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
            _speedMoveDownCalled = false; // Reset flag when state changes
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            if (_lift->isBallWaiting() && !_speedMoveDownCalled)
            {
                _lift->down();
                _speedMoveDownCalled = true;
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_UNLOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            if (_lift->isBallWaiting())
            {
                MLOG_INFO("AutoMode: Loading ball into lift");
                _lift->loadBall();
            }

            // Sometimes with random delay, try to load a ball
            // Maybe switch is not detected properly
            else
            {
                static ulong liftIdleStartTime = 0;
                static int randomLiftDelayMs = 0;
                static bool liftDelaySet = false;

                if (!liftDelaySet)
                {
                    liftIdleStartTime = currentMillis;
                    randomLiftDelayMs = random(20000, 180000); // 10s to 60s
                    liftDelaySet = true;
                }
                else if (currentMillis >= liftIdleStartTime + randomLiftDelayMs)
                {
                    MLOG_INFO("AutoMode: Random starting lift, for case the sensor didn't detect a ball");
                    _lift->loadBall();
                    liftDelaySet = false;
                }
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->up();
            break;
        case devices::LiftStateEnum::LIFT_UP_LOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->unloadBall();
            break;
        case devices::LiftStateEnum::LIFT_UP_UNLOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->down(_lift->isBallWaiting() ? 1.0f : 0.25f);
            break;
        default:
            MLOG_WARN("AutoMode: Unknown lift state");
            break;
        }
}
