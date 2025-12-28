/**
 * @file Test2.h
 * @brief Test2 device based on DeviceBase with Led and Button children
 */

#ifndef COMPOSITION_TEST2_H
#define COMPOSITION_TEST2_H

#include "Logging.h"
#include "devices/composition/DeviceBase.h"
#include "devices/composition/Led.h"
#include "devices/composition/Button.h"
#include "devices/composition/Servo.h"

namespace composition {

class Test2 : public DeviceBase {
private:
    Led* _led;
    Button* _button;
    Servo* _servo;

public:
    explicit Test2(const String &id);

    void setup() override;
    void update();

    // Getters for child devices
    Led* getLed();
    Button* getButton();
    Servo* getServo();
};

} // namespace composition

#endif // COMPOSITION_TEST2_H
