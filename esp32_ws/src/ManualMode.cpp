#include "ManualMode.h"
#include "Logging.h"
#include "devices/composition/Button.h"
#include "devices/composition/Buzzer.h"
#include "devices/composition/Wheel.h"
#include "devices/composition/Led.h"

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
    // TODO: Re-enable when legacy devices are converted to composition devices
    // _wheelNextBtn = deviceManager.getDeviceByIdAs<composition::Button>("wheel-next-btn");
    // if (_wheelNextBtn == nullptr)
    // {
    //     MLOG_ERROR("Required device 'wheel-next-btn' not found!");
    // }
    // else
    // {
    //     // Second call: unsubscribe from previous callback
    //     if (_wheelNextBtnUnsubscribe)
    //     {
    //         _wheelNextBtnUnsubscribe();
    //         _wheelNextBtnUnsubscribe = nullptr;
    //     }

    //     // First call: subscribe to button state changes
    //     _wheelNextBtnUnsubscribe = _wheelNextBtn->onStateChange([this](void *data)
    //                                                             {
    //                                                                     // Handle button press - move to next breakpoint if wheel is idle
    //                                                                     if (_wheelNextBtn &&  _wheelNextBtn->isPressed())
    //                                                                     {
    //                                                                         if (_buzzer != nullptr)
    //                                                                             _buzzer->tone(1000, 100);

    //                                                                     } });

    //     _wheel = deviceManager.getDeviceByIdAs<composition::Wheel>("wheel");
    //     if (_wheel == nullptr)
    //     {
    //         MLOG_ERROR("Required device 'wheel' not found!");
    //     }

    //     _buzzer = deviceManager.getDeviceByIdAs<composition::Buzzer>("buzzer");
    //     if (_buzzer == nullptr)
    //     {
    //         MLOG_ERROR("Required device 'buzzer' not found!");
    //     }

    //     _wheelBtnLed = deviceManager.getDeviceByIdAs<composition::Led>("wheel-btn-led");
    //     if (_wheelBtnLed == nullptr)
    //     {
    //         MLOG_ERROR("Required device 'wheel-btn-led' not found!");
    //     }

    //     // _splitter = deviceManager.getDeviceByIdAs<Wheel>("splitter");
    //     // if (_splitter == nullptr)
    //     // {
    //     //     MLOG_ERROR("Required device 'splitter' not found!");
    //     // }

    //     // _splitterNextBtn = deviceManager.getDeviceByIdAs<Button>("splitter-next-btn");
    //     // if (_splitterNextBtn == nullptr)
    //     // {
    //     //     MLOG_ERROR("Required device 'splitter-next-btn' not found!");
    //     // }

    //     // _splitterBtnLed = deviceManager.getDeviceByIdAs<Led>("splitter-btn-led");
    //     // if (_splitterBtnLed == nullptr)
    //     // {
    //     //     MLOG_ERROR("Required device 'splitter-btn-led' not found!");
    //     // }

    //     MLOG_INFO("ManualMode setup complete");
    // }
}

void ManualMode::loop()
{
    // TODO: Re-enable when legacy devices are converted to composition devices
    // ManualMode is currently disabled
    return;

    bool ledBlinkFast = millis() % 500 > 250;
    bool ledBlinkSlow = millis() % 1000 > 500;

    // Wheel Button
    /*
    if (_wheelNextBtn && _wheel && _wheelNextBtn->isPressed())
    {
        auto wheelState = _wheel->getState();
        if (wheelState.state == composition::WheelStateEnum::IDLE || wheelState.state == composition::WheelStateEnum::UNKNOWN)
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
            auto wheelState = _wheel->getState();

            switch (wheelState.state)
            {
            case composition::WheelStateEnum::MOVING:
            case composition::WheelStateEnum::CALIBRATING:
            case composition::WheelStateEnum::INIT:
                _wheelBtnLed->set(ledBlinkFast);
                break;
            case composition::WheelStateEnum::ERROR:
                _wheelBtnLed->set(false);
                break;
            case composition::WheelStateEnum::UNKNOWN:
            case composition::WheelStateEnum::IDLE:
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
    if (_wheel && _wheel->getState().onError && _buzzer)
    {
        _buzzer->tone(1000, 500); // Long buzz for error
    }

    */

    // // Splitter Button
    // if (_splitterNextBtn && _splitter && _splitterNextBtn->isPressed())
    // {
    //     if (_splitter->getState().state == composition::WheelStateEnum::IDLE || _splitter->getState().state == composition::WheelStateEnum::UNKNOWN)
    //     {
    //         if (_buzzer != nullptr)
    //             _buzzer->tone(200, 100);
    //         _splitter->nextBreakPoint();
    //     }
    //     else
    //     {
    //         MLOG_WARN("Splitter [%s]: Next button pressed, but splitter ignored, wheel not idle", _splitter->getId().c_str());
    //     }
    // }

    // // Splitter Led
    // if (_splitterBtnLed)
    // {
    //     if (_splitter)
    //     {
    //         switch (_splitter->getState().state)
    //         {
    //         case composition::WheelStateEnum::MOVING:
    //         case composition::WheelStateEnum::CALIBRATING:
    //         case composition::WheelStateEnum::INIT:
    //             _splitterBtnLed->set(ledBlinkFast);
    //             break;
    //         case composition::WheelStateEnum::ERROR:
    //             _splitterBtnLed->set(false);
    //             break;
    //         case composition::WheelStateEnum::UNKNOWN:
    //         case composition::WheelStateEnum::IDLE:
    //             _splitterBtnLed->set(true);
    //             break;
    //         default:
    //             MLOG_ERROR("ManualMode: Unknown wheel state");
    //             break;
    //         }
    //     }
    //     else
    //     {
    //         _wheelBtnLed->set(false);
    //     }
    // }

    // // Check for splitter error and buzz
    // if (_splitter && _splitter->getState().onError && _buzzer)
    // {
    //     _buzzer->tone(1000, 500); // Long buzz for error
    // }
}