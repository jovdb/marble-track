#include "ManualMode.h"
#include "Logging.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"

ManualMode::ManualMode(DeviceManager &deviceManager) : deviceManager(deviceManager), _wheelNextBtn(nullptr), _wheel(nullptr), _buzzer(nullptr), _wheelBtnLed(nullptr)
{
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

    MLOG_INFO("ManualMode setup complete");
}

void ManualMode::loop()
{
    bool ledBlinkFast = millis() % 500 > 250;
    bool ledBlinkSlow = millis() % 1000 > 500;

    if (_wheelNextBtn && _wheel && _wheelNextBtn->isPressed())
    {
        if (_buzzer != nullptr)
            _buzzer->tone(200, 100);
        _wheel->nextBreakPoint();
    }

    // Wheel Led
    if (_wheel && _wheel->wheelState != Wheel::WheelState::IDLE)
    {
        if (_wheelBtnLed)
        {
            _wheelBtnLed->set(ledBlinkSlow);
        }
    }
    else
    {
        if (_wheelBtnLed)
            _wheelBtnLed->set(true); // Solid on when idle
    }
}