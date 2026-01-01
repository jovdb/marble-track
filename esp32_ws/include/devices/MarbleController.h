#ifndef MARBLECONTROLLER_H
#define MARBLECONTROLLER_H

#include <Arduino.h>
#include <functional>
#include "Device.h"
#include "devices/Button.h"
#include "devices/Wheel.h"
#include "devices/Buzzer.h"
#include "devices/Led.h"
#include "devices/Lift.h"

namespace devices
{

    class MarbleController : public Device
    {
    public:
        MarbleController(const String &id);
        void loop() override;

        /**
         * @brief Play an error sound using the buzzer
         */
        void playErrorSound();

    private:
        void loopLift();
        devices::Buzzer *_buzzer;
        devices::Lift *_lift;
        devices::Led *_liftLed;
        devices::Button *_liftButton;
        devices::Button *_manualButton;

        // Button timing for unload duration control
        unsigned long _liftButtonPressStartTime = 0;
        bool _waitingForLiftButtonRelease = false;
    };

} // namespace devices

#endif // MARBLECONTROLLER_H
