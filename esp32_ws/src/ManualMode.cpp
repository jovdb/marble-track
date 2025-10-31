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
    _wheel = deviceManager.getDeviceByIdAs<Wheel>("wheel");
    _buzzer = deviceManager.getDeviceByIdAs<Buzzer>("buzzer");
    _wheelBtnLed = deviceManager.getDeviceByIdAs<Led>("wheel-btn-led");
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

    if (_wheel && _wheel->wheelState == Wheel::WheelState::MOVING)
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