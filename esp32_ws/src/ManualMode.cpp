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

    _lift = nullptr;
    _liftButton = nullptr;
    _liftLed = nullptr;
}

ManualMode::~ManualMode()
{
    if (_wheelNextBtnUnsubscribe)
    {
        _wheelNextBtnUnsubscribe();
    }
    if (_liftButtonUnsubscribe)
    {
        _liftButtonUnsubscribe();
    }
}

void ManualMode::setup()
{

    MLOG_DEBUG("ManualMode: Starting setup");

    _buzzer = deviceManager.getDeviceByIdAs<devices::Buzzer>("buzzer");
    if (_buzzer == nullptr)
    {
        MLOG_ERROR("Required device 'buzzer' not found!");
    }
    else
    {
        _buzzer->tune("scale_up:d=32,o=5,b=300:c,c#,d#,e,f#,g#,a#,b");
    }

    // TODO: Re-enable when legacy devices are converted to composition devices
    // _wheelNextBtn = deviceManager.getDeviceByIdAs<devices::Button>("wheel-next-btn");
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

    //     _wheel = deviceManager.getDeviceByIdAs<devices::Wheel>("wheel");
    //     if (_wheel == nullptr)
    //     {
    //         MLOG_ERROR("Required device 'wheel' not found!");
    //     }

    //     _buzzer = deviceManager.getDeviceByIdAs<devices::Buzzer>("buzzer");
    //     if (_buzzer == nullptr)
    //     {
    //         MLOG_ERROR("Required device 'buzzer' not found!");
    //     }

    //     _wheelBtnLed = deviceManager.getDeviceByIdAs<devices::Led>("wheel-btn-led");
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

    _lift = deviceManager.getDeviceByIdAs<devices::Lift>("lift");
    if (_lift == nullptr)
    {
        MLOG_ERROR("Required device 'lift' not found!");
    }

    if (_liftButton && _liftButtonUnsubscribe)
    {
        _liftButtonUnsubscribe();
        _liftButtonUnsubscribe = nullptr;
    }
    _liftButton = deviceManager.getDeviceByIdAs<devices::Button>("lift-btn");
    if (_liftButton == nullptr)
    {
        MLOG_ERROR("Required device 'lift-btn' not found!");
    }
    else
    {
        _liftButtonUnsubscribe = _liftButton->onStateChange([this](void *data)
                                                            { Serial.println("Lift button state changed"); });
        if (_buzzer)
            _buzzer->tone(1000, 100);

        if (_lift)
        {
            if (_lift->isInitialized())
            {
                _lift->loadBall();
            }
            else
            {
                _lift->init();
            }
            //        _lift->getState().
        }
    }

    _liftLed = deviceManager.getDeviceByIdAs<devices::Led>("lift-led");
    if (_liftLed == nullptr)
    {
        MLOG_ERROR("Required device 'lift-led' not found!");
    }
}

void ManualMode::loop()
{
    bool ledBlinkFast = millis() % 500 > 250;
    bool ledBlinkSlow = millis() % 1000 > 500;

    // Wheel Button
    /*
    if (_wheelNextBtn && _wheel && _wheelNextBtn->isPressed())
    {
        auto wheelState = _wheel->getState();
        if (wheelState.state == devices::WheelStateEnum::IDLE || wheelState.state == devices::WheelStateEnum::UNKNOWN)
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
            case devices::WheelStateEnum::MOVING:
            case devices::WheelStateEnum::CALIBRATING:
            case devices::WheelStateEnum::INIT:
                _wheelBtnLed->set(ledBlinkFast);
                break;
            case devices::WheelStateEnum::ERROR:
                _wheelBtnLed->set(false);
                break;
            case devices::WheelStateEnum::UNKNOWN:
            case devices::WheelStateEnum::IDLE:
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
    //     if (_splitter->getState().state == devices::WheelStateEnum::IDLE || _splitter->getState().state == devices::WheelStateEnum::UNKNOWN)
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
    //         case devices::WheelStateEnum::MOVING:
    //         case devices::WheelStateEnum::CALIBRATING:
    //         case devices::WheelStateEnum::INIT:
    //             _splitterBtnLed->set(ledBlinkFast);
    //             break;
    //         case devices::WheelStateEnum::ERROR:
    //             _splitterBtnLed->set(false);
    //             break;
    //         case devices::WheelStateEnum::UNKNOWN:
    //         case devices::WheelStateEnum::IDLE:
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
