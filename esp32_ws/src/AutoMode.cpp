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

    _wheel = deviceManager.getDeviceByIdAs<Wheel>("wheel");
    if (_wheel == nullptr)
    {
        MLOG_ERROR("Required device 'wheel' not found!");
    }

    _buzzer = deviceManager.getDeviceByIdAs<Buzzer>("buzzer");
    if (_buzzer == nullptr)
    {
        MLOG_ERROR("Required device 'buzzer' not found!");
    }

    _wheelBtnLed = deviceManager.getDeviceByIdAs<Led>("wheel-btn-led");
    if (_wheelBtnLed == nullptr)
    {
        MLOG_ERROR("Required device 'wheel-btn-led' not found!");
    }

    _splitter = deviceManager.getDeviceByIdAs<Wheel>("splitter");
    if (_splitter == nullptr)
    {
        MLOG_ERROR("Required device 'splitter' not found!");
    }

    _splitterBtnLed = deviceManager.getDeviceByIdAs<Led>("splitter-btn-led");
    if (_splitterBtnLed == nullptr)
    {
        MLOG_ERROR("Required device 'splitter-btn-led' not found!");
    }

    _lift = deviceManager.getDeviceByIdAs<Lift>("lift");
    if (_lift == nullptr)
    {
        MLOG_ERROR("Required device 'lift' not found!");
    }

    MLOG_INFO("AutoMode setup complete");
}

void AutoMode::loop()
{

    const ulong currentMillis = millis();

    // Reset devices at start
    if (_wheel && _wheel->wheelState == Wheel::WheelState::UNKNOWN)
    {
        MLOG_INFO("Initializing Wheel");
        _wheel->init();
    }

    static ulong liftInitStartTime = 0;
    if (_lift && _lift->liftState == Lift::LiftState::UNKNOWN)
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
    if (_splitter && _splitter->wheelState == Wheel::WheelState::UNKNOWN)
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
    if (_wheel && _wheel->onCurrentBreakpointIndexChanged)
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
            (_wheel && _wheel->wheelState != Wheel::WheelState::UNKNOWN) &&
            (_splitter && _splitter->wheelState != Wheel::WheelState::UNKNOWN) &&
            (_lift && _lift->liftState != Lift::LiftState::UNKNOWN);

        if (!allInitialized)
            return;
    */
    // Wheel
    static ulong wheelIdleStartTime = 0;
    static int randomWheelDelayMs = 0;
    static bool wheelDelaySet = false;

    if (_wheel)
        switch (_wheel->wheelState)
        {
        case Wheel::WheelState::UNKNOWN:
        case Wheel::WheelState::CALIBRATING:
        case Wheel::WheelState::INIT:
        case Wheel::WheelState::ERROR:
            wheelDelaySet = false;
            break;
        case Wheel::WheelState::MOVING:
            wheelDelaySet = false;
            break;
        case Wheel::WheelState::IDLE:
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
        switch (_splitter->wheelState)
        {
        case Wheel::WheelState::UNKNOWN:
        case Wheel::WheelState::CALIBRATING:
        case Wheel::WheelState::INIT:
        case Wheel::WheelState::MOVING:
        case Wheel::WheelState::ERROR:
            break;
        case Wheel::WheelState::IDLE:
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
        switch (_lift->liftState)
        {
        case Lift::LiftState::UNKNOWN:
        case Lift::LiftState::INIT:
        case Lift::LiftState::ERROR:
        case Lift::LiftState::MOVING_UP:
        case Lift::LiftState::LIFT_DOWN_LOADING:
        case Lift::LiftState::LIFT_UP_UNLOADING:
            _speedMoveDownCalled = false; // Reset flag when state changes
            break;
        case Lift::LiftState::MOVING_DOWN:
            if (_lift->isBallWaiting() && !_speedMoveDownCalled)
            {
                _lift->down();
                _speedMoveDownCalled = true;
            }
            break;
        case Lift::LiftState::LIFT_DOWN_UNLOADED:
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
        case Lift::LiftState::LIFT_DOWN_LOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->up();
            break;
        case Lift::LiftState::LIFT_UP_LOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->unloadBall();
            break;
        case Lift::LiftState::LIFT_UP_UNLOADED:
            _speedMoveDownCalled = false; // Reset flag when state changes
            _lift->down(_lift->isBallWaiting() ? 1.0f : 0.25f);
            break;
        default:
            MLOG_WARN("AutoMode: Unknown lift state");
            break;
        }
}