/**
 * @file Test2.h
 * @brief Test2 device based on Device with Led and Button children
 */

#ifndef COMPOSITION_TEST2_H
#define COMPOSITION_TEST2_H

#include "Logging.h"
#include "devices/Device.h"
#include "devices/Led.h"
#include "devices/Button.h"
#include "devices/Servo.h"
#include <functional>

namespace devices {

class Test2 : public Device {
private:
    Led* _led;
    Button* _button;
    Servo* _servo;
    std::function<void()> _buttonUnsubscribe;

public:
    explicit Test2(const String &id);

    void setup() override;
    void update();

    // Getters for child devices
    Led* getLed();
    Button* getButton();
    Servo* getServo();
};

} // namespace devices

#endif // COMPOSITION_TEST2_H
