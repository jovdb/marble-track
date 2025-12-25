/**
 * @file Test2.h
 * @brief Test2 device based on DeviceBase with Led and Button children
 */

#ifndef COMPOSITION_TEST2_H
#define COMPOSITION_TEST2_H

#include "devices/composition/DeviceBase.h"

namespace composition {

class Led;
class Button;

class Test2 : public DeviceBase {
public:
    explicit Test2(const String &id);

    void setup() override;
    void loop() override;

private:
    Led* _led;
    Button* _button;
};

} // namespace composition

#endif // COMPOSITION_TEST2_H
