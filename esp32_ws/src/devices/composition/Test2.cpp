/**
 * @file Test2.cpp
 * @brief Test2 device implementation with Led and Button children
 */

#include "devices/composition/Test2.h"
#include "devices/composition/Led.h"
#include "devices/composition/Button.h"

namespace composition {

Test2::Test2(const String &id)
    : DeviceBase(id, "test2")
{
    // Create and add child LED device
    _led = new Led(id + "_led");
    addChild(_led);

    // Create and add child Button device
    _button = new Button(id + "_button");
    addChild(_button);
}

void Test2::setup() {
    // Call base to setup all children
    DeviceBase::setup();
}

void Test2::loop() {
    // Call base to loop all children
    DeviceBase::loop();
}

} // namespace composition
