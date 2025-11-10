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
    if (_wheelNextBtn && _wheel && _wheelNextBtn->onPressed())
    {
        if (_wheel->wheelState == Wheel::WheelState::IDLE || _wheel->wheelState == Wheel::WheelState::UNKNOWN)
        {
            if (_buzzer != nullptr)
                _buzzer->tone(200, 100);
            _wheel->nextBreakPoint();
        }
        else
        {
            MLOG_WARN("Wheel [%s]: Next button pressed, but wheel ignored, wheel not idle", _wheel->getId().c_str());
        }
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
            case Wheel::WheelState::INIT:
                _wheelBtnLed->set(ledBlinkFast);
                break;
            case Wheel::WheelState::ERROR:
                _wheelBtnLed->set(false);
                break;
            case Wheel::WheelState::UNKNOWN:
            case Wheel::WheelState::IDLE:
                _wheelBtnLed->set(true);
                break;

            default:
                MLOG_ERROR("ManualMode: Unknown wheel state");
                break;
            }
        }
        else
        {
            _wheelBtnLed->set(false);
        }
    }

    // Check for wheel error and buzz
    if (_wheel && _wheel->getOnError() && _buzzer)
    {
        _buzzer->tone(1000, 500); // Long buzz for error
    }

    // Splitter Button
    if (_splitterNextBtn && _splitter && _splitterNextBtn->onPressed())
    {
        if (_splitter->wheelState == Wheel::WheelState::IDLE || _splitter->wheelState == Wheel::WheelState::UNKNOWN)
        {
            if (_buzzer != nullptr)
                _buzzer->tone(200, 100);
            _splitter->nextBreakPoint();
        }
        else
        {
            MLOG_WARN("Splitter [%s]: Next button pressed, but splitter ignored, wheel not idle", _splitter->getId().c_str());
        }
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
            case Wheel::WheelState::INIT:
                _splitterBtnLed->set(ledBlinkFast);
                break;
            case Wheel::WheelState::ERROR:
                _splitterBtnLed->set(false);
                break;
            case Wheel::WheelState::UNKNOWN:
            case Wheel::WheelState::IDLE:
                _splitterBtnLed->set(true);
                break;
            default:
                MLOG_ERROR("ManualMode: Unknown wheel state");
                break;
            }
        }
        else
        {
            _wheelBtnLed->set(false);
        }
    }

    // Check for splitter error and buzz
    if (_splitter && _splitter->getOnError() && _buzzer)
    {
        _buzzer->tone(1000, 500); // Long buzz for error
    }
}