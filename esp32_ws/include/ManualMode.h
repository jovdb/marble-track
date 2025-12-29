#ifndef MANUALMODE_H
#define MANUALMODE_H

#include <Arduino.h>
#include <functional>
#include "DeviceManager.h"
#include "devices/Button.h"
#include "devices/Wheel.h"
#include "devices/Buzzer.h"
#include "devices/Led.h"
#include "devices/Lift.h"

class ManualMode
{
public:
    ManualMode(DeviceManager &deviceManager);
    ~ManualMode();
    void setup();
    void loop();

    /**
     * @brief Get the current state of the lift
     * @return LiftState structure or nullptr if lift not available
     */
    const devices::LiftState *getLiftState() const;

    /**
     * @brief Check if the lift is initialized
     * @return true if lift is initialized, false otherwise
     */
    bool isLiftInitialized() const;

private:
    DeviceManager &deviceManager;
    devices::Button *_wheelNextBtn;
    devices::Wheel *_wheel;
    devices::Buzzer *_buzzer;
    devices::Led *_wheelBtnLed;
    devices::Button *_splitterNextBtn;
    devices::Led *_splitterBtnLed;
    devices::Wheel *_splitter;
    devices::Lift *_lift;
    devices::Button *_liftButton;
    devices::Led *_liftLed;
    std::function<void()> _wheelNextBtnUnsubscribe;
    std::function<void()> _liftButtonUnsubscribe;
};

#endif // MANUALMODE_H
