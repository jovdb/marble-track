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

namespace composition {

class Test2 : public DeviceBase {
public:
    explicit Test2(const String &id);

    void setup() override;
    void update();

    // Getters for child devices
    Led* getLed() { return _led; }
    Button* getButton() { return _button; }

private:
    Led* _led;
    Button* _button;
};

} // namespace composition

#endif // COMPOSITION_TEST2_H
