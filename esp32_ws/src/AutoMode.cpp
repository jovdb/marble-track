#include "AutoMode.h"
#include "Logging.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"

AutoMode::AutoMode(DeviceManager &deviceManager) : deviceManager(deviceManager)
{
    _buzzer = nullptr;

    _wheel = nullptr;
    _wheelBtnLed = nullptr;

    _splitter = nullptr;
    _splitterBtnLed = nullptr;
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

    MLOG_INFO("AutoMode setup complete");
}

void AutoMode::loop()
{
    // Reset devices at start
    if (_wheel->wheelState == Wheel::WheelState::UNKNOWN)
    {
        MLOG_INFO("Initializing Wheel");
        _wheel->reset();
    }

    if (_splitter->wheelState == Wheel::WheelState::UNKNOWN)
    {
        MLOG_INFO("Initializing Stepper");
        _splitter->reset();
    }

    const ulong currentMillis = millis();
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
    if (_wheel->onCurrentBreakpointIndexChanged)
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

    const bool allInitialized =
        (_wheel && _wheel->wheelState != Wheel::WheelState::UNKNOWN) &&
        (_splitter && _splitter->wheelState != Wheel::WheelState::UNKNOWN);

    if (!allInitialized)
        return;

    // Wheel
    static ulong wheelIdleStartTime = 0;
    static int randomWheelDelayMs = 0;
    static bool wheelDelaySet = false;

    switch (_wheel->wheelState)
    {
    case Wheel::WheelState::UNKNOWN:

    case Wheel::WheelState::CALIBRATING:
    case Wheel::WheelState::RESET:
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
            randomWheelDelayMs = random(100, 30000); // random(min, max) is [min, max)
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
    switch (_splitter->wheelState)
    {
    case Wheel::WheelState::UNKNOWN:
    case Wheel::WheelState::CALIBRATING:
    case Wheel::WheelState::RESET:
    case Wheel::WheelState::MOVING:
    case Wheel::WheelState::ERROR:
        break;
    case Wheel::WheelState::IDLE:
        if (wheelExitMaxBallCount > 0 && currentMillis > lastWheelExitTime + 1500)
        {
            MLOG_INFO("AutoMode: Triggering splitter 1.5s after wheel ball unloading, remaining count %d", wheelExitMaxBallCount);
            wheelExitMaxBallCount--;
            _splitter->nextBreakPoint();
        }
        break;
    default:
        MLOG_WARN("AutoMode: Unknown splitter state");
        break;
    }
}