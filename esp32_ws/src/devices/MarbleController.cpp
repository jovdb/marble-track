#include "devices/MarbleController.h"
#include "Logging.h"
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Buzzer.h"
#include "devices/Wheel.h"
#include "devices/Led.h"

extern DeviceManager deviceManager;

namespace devices
{

    MarbleController::MarbleController(const String &id) : Device(id, "MarbleController")
    {
        _buzzer = new devices::Buzzer("buzzer");
        JsonDocument buzzerConfigDoc;
        buzzerConfigDoc["name"] = "Buzzer";
        buzzerConfigDoc["pin"] = 14;
        _buzzer->jsonToConfig(buzzerConfigDoc);
        addChild(_buzzer);

        _lift = new devices::Lift("lift");
        JsonDocument liftConfigDoc;
        liftConfigDoc["name"] = "Lift";
        liftConfigDoc["minSteps"] = 0;
        liftConfigDoc["maxSteps"] = 1500;
        _lift->jsonToConfig(liftConfigDoc);
        addChild(_lift);

        _liftLed = new devices::Led("lift-led");
        JsonDocument configDoc;
        configDoc["name"] = "Lift Led";
        configDoc["pin"] = 15;
        _liftLed->jsonToConfig(configDoc);
        addChild(_liftLed);

        _liftButton = new devices::Button("lift-button");
        JsonDocument buttonConfigDoc;
        buttonConfigDoc["name"] = "Lift Button";
        buttonConfigDoc["pin"] = 16;
        buttonConfigDoc["pinMode"] = "pullup";
        buttonConfigDoc["debounceTimeInMs"] = 50;
        buttonConfigDoc["buttonType"] = "NormalOpen";
        _liftButton->jsonToConfig(buttonConfigDoc);
        addChild(_liftButton);
    }

    void MarbleController::loop()
    {
        Device::loop();

        loopLift();
    }

    void MarbleController::loopLift()
    {
        // Lift control logic

        auto liftState = _lift->getState();

        switch (liftState.state)
        {
        case devices::LiftStateEnum::UNKNOWN:
            // Wait for initialization
            _liftLed->set(false);
            _lift->init();
            break;

        case devices::LiftStateEnum::ERROR:
            // Handle error state - maybe blink LED faster
            _liftLed->set(false);

            // Play sound for new errors
            if (liftState.onErrorChange)
            {
                playErrorSound();
            }
            break;

        // BUSY
        case devices::LiftStateEnum::INIT:
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
        case devices::LiftStateEnum::MOVING_UP:
        case devices::LiftStateEnum::MOVING_DOWN: // Loading in progress
            _liftLed->blink(500, 500);
            break;

        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
            _liftLed->set(true);
            if (_liftButton->isPressed())
            {
                _lift->up();
            }
            break;

        case devices::LiftStateEnum::LIFT_DOWN_UNLOADED:
            _liftLed->set(true);
            if (_liftButton->isPressed())
            {
                _lift->loadBall();
            }
            break;

        case devices::LiftStateEnum::LIFT_UP_UNLOADED:
            _liftLed->set(true);
            if (_liftButton->isPressed())
            {
                _lift->down();
            }
            break;

        case devices::LiftStateEnum::LIFT_UP_LOADED:
            _liftLed->set(true);
            if (_liftButton->isPressed())
            {
                _lift->unloadBall();
            }
            break;
        }
    }

    void MarbleController::playErrorSound()
    {
        if (_buzzer)
        {
            // _buzzer->tone(100, 800); // Play a 100ms tone at 800Hz
            _buzzer->tune("Error:d=4,o=4,b=100:a,d"); // Play error tune
        }
    }

} // namespace devices
