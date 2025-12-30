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

        // bool ledBlinkFast = millis() % 500 > 250;
        // bool ledBlinkSlow = millis() % 1000 > 500;

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
            break;
        case devices::LiftStateEnum::INIT:
            // Initialization in progress
            _liftLed->blink(500, 500);
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADING:
            // Loading in progress
            _liftLed->blink(500, 500);
            break;
        case devices::LiftStateEnum::LIFT_DOWN_LOADED:
            // Ball is loaded at bottom, unload when button pressed
            _liftLed->set(true);

            if (_liftButton->isPressed())
            {
                _lift->up();
            }
            break;
        case devices::LiftStateEnum::LIFT_DOWN_UNLOADED:
            // No ball loaded at bottom, load when button pressed
            if (_liftButton->isPressed())
            {
                _lift->loadBall();
            }
            break;
        case devices::LiftStateEnum::LIFT_UP_UNLOADING:
            _liftLed->blink(500, 500);

            break;
        case devices::LiftStateEnum::LIFT_UP_UNLOADED:
            _liftLed->set(true);

            // Ball unloaded at top, move down when button pressed
            if (_liftButton->isPressed())
            {
                _lift->down();
            }
            break;
        case devices::LiftStateEnum::LIFT_UP_LOADED:
            // Ball loaded at top, unload when button pressed
            _liftLed->set(true);

            if (_liftButton->isPressed())
            {
                _lift->unloadBall();
            }
            break;
        case devices::LiftStateEnum::MOVING_UP:
            _liftLed->blink(500, 500);

            // Moving up, wait for completion
            break;
        case devices::LiftStateEnum::MOVING_DOWN:
            _liftLed->blink(500, 500);

            // Moving down, wait for completion
            break;
        default:
            break;
        }
    }

} // namespace devices
