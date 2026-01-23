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
        void setup() override;
        void loop() override;

        /**
         * @brief Play an error sound using the buzzer
         */
        void playErrorSound();

        /**
         * @brief Play a click sound using the buzzer
         */
        void playClickSound();

        /**
         * @brief Play a startup sound using the buzzer
         */
        void playStartupSound();

    private:
        void loopManualLift();
        void loopManualWheel();
        void loopAutoLift();
        void loopAutoWheel();
        Button *_manualButton;
        Buzzer *_buzzer;
        Lift *_lift;
        Wheel *_wheel;
        Led *_liftLed;
        Button *_liftBtn;
        Led *_wheelLed;
        Button *_wheelBtn;

        // Button timing for unload duration control
        unsigned long _liftButtonPressStartTime = 0;
        bool _isBallStillLoaded = false;
        
        // Auto lift timing control
        unsigned long _autoLiftDelayStart = 0;
        unsigned long _autoLiftDelayMs = 1000; // 1 second delay between auto operations
        
        // Auto wheel timing control
        unsigned long _wheelIdleStartTime = 0;
        unsigned long _randomWheelDelayMs = 0;
        
        bool isAutoMode = false;
    };

} // namespace devices

#endif // MARBLECONTROLLER_H
