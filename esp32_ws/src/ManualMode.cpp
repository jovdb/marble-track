#include "ManualMode.h"
#include "Logging.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"

ManualMode::ManualMode(DeviceManager &deviceManager) : deviceManager(deviceManager)
{
    _buzzer = nullptr;

    _wheel = nullptr;
    _wheelNextBtn = nullptr;
    _wheelBtnLed = nullptr;

    _splitter = nullptr;
    _splitterNextBtn = nullptr;
    _splitterBtnLed = nullptr;
}

void ManualMode::setup()
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

    MLOG_INFO("ManualMode setup complete");
}

void ManualMode::loop()
{
    bool ledBlinkFast = millis() % 500 > 250;
    bool ledBlinkSlow = millis() % 1000 > 500;

    // Wheel Button
    if (_wheelNextBtn && _wheel && _wheelNextBtn->isPressed() && _wheel->wheelState == Wheel::WheelState::IDLE)
    {
        if (_buzzer != nullptr)
            _buzzer->tone(200, 100);
        _wheel->nextBreakPoint();
    }

    // Wheel Led
    if (_wheelBtnLed)
    {
        if (_wheel)
        {
            switch (_wheel->wheelState)
            {
            case Wheel::WheelState::MOVING:
            case Wheel::WheelState::CALIBRATING:
            case Wheel::WheelState::RESET:
                _wheelBtnLed->set(ledBlinkFast);
                break;
            case Wheel::WheelState::ERROR:
                _wheelBtnLed->set(false);
                break;
            case Wheel::WheelState::IDLE:
                _wheelBtnLed->set(true);
                break;
            }
        }
        else
        {
            _wheelBtnLed->set(false);
        }
    }

    // Splitter Button
    if (_splitterNextBtn && _splitter && _splitterNextBtn->isPressed() && _splitter->wheelState == Wheel::WheelState::IDLE)
    {
        if (_buzzer != nullptr)
            _buzzer->tone(200, 100);
        _splitter->nextBreakPoint();
    }

    // Splitter Led
    if (_splitterBtnLed)
    {
        if (_splitter)
        {
            switch (_splitter->wheelState)
            {
            case Wheel::WheelState::MOVING:
            case Wheel::WheelState::CALIBRATING:
            case Wheel::WheelState::RESET:
                _splitterBtnLed->set(ledBlinkFast);
                break;
            case Wheel::WheelState::ERROR:
                _splitterBtnLed->set(false);
                break;
            case Wheel::WheelState::IDLE:
                _splitterBtnLed->set(true);
                break;
            }
        }
        else
        {
            _wheelBtnLed->set(false);
        }
    }
}