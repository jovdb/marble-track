#ifndef MARBLECONTROLLER_H
#define MARBLECONTROLLER_H

#include <Arduino.h>
#include <functional>
#include "Device.h"
#include "DeviceManager.h"
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
    ~MarbleController();
    void setup() override;
    void loop() override;

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
    DeviceManager *_deviceManager;
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

} // namespace devices

#endif // MARBLECONTROLLER_H
