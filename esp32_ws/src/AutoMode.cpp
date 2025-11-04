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
    _wheelNextBtn = nullptr;
    _wheelBtnLed = nullptr;

    _splitter = nullptr;
    _splitterNextBtn = nullptr;
    _splitterBtnLed = nullptr;
}

void AutoMode::setup()
{
    _wheelNextBtn = deviceManager.getDeviceByIdAs<Button>("wheel-next-btn");
    if (_wheelNextBtn == nullptr)
    {
        MLOG_ERROR("Required device 'wheel-next-btn' not found!");
    }

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

    _splitterNextBtn = deviceManager.getDeviceByIdAs<Button>("splitter-next-btn");
    if (_splitterNextBtn == nullptr)
    {
        MLOG_ERROR("Required device 'splitter-next-btn' not found!");
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
    switch (_wheel->wheelState)
    {
    case Wheel::WheelState::IDLE:
        _wheel->reset();
        break;
    case Wheel::WheelState::CALIBRATING:
    case Wheel::WheelState::RESET:
    case Wheel::WheelState::MOVING:
    case Wheel::WheelState::ERROR:
        // Handle calibrating/reset state
        break;
    default:
        MLOG_WARN("AutoMode: Unknown wheel state");
        break;
    }

    switch (_splitter->wheelState)
    {
    case Wheel::WheelState::IDLE:
        _splitter->reset();
        break;
    case Wheel::WheelState::CALIBRATING:
    case Wheel::WheelState::RESET:
    case Wheel::WheelState::MOVING:
    case Wheel::WheelState::ERROR:
        // Handle calibrating/reset state
        break;
    default:
        MLOG_WARN("AutoMode: Unknown splitter state");
        break;
    }

    const ulong currentTime = millis();

    // Loop button leds
    const byte stepCount = 3;
    const int stepDurationMs = 500;

    int step = static_cast<int>(std::floor((currentTime % (stepCount * stepDurationMs)) / stepDurationMs));
    if (_wheelBtnLed)
        _wheelBtnLed->set(step == 1);
    if (_splitterBtnLed)
        _splitterBtnLed->set(step == 2);
}