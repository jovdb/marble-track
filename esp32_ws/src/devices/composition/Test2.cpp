/**
 * @file Test2.cpp
 * @brief Test2 device implementation with Led and Button children
 */

#include "devices/composition/Test2.h"
#include "devices/composition/Led.h"
#include "devices/composition/Button.h"
#include "devices/composition/Servo.h"

namespace composition
{

    Test2::Test2(const String &id)
        : DeviceBase(id, "test2")
    {
        // Create and add child LED device
        _led = new Led(id + "_led");
        addChild(_led);

        // Create and add child Button device
        _button = new Button(id + "_button");
        addChild(_button);

        // Create and add child Servo device
        _servo = new Servo(id + "_servo");
        addChild(_servo);
    }

    void Test2::setup()
    {
        // Call base to setup all children
        DeviceBase::setup();

        // Only check if button state changes, if we need to do something
        _button->onStateChange([this](void *data)
                               { update(); });
    }

    void Test2::update()
    {
        auto btnState = _button->getState();
        auto ledState = _led->getState();

        if (btnState.isPressed && ledState.mode != "BLINKING")
        {
            _led->blink(100, 100);
            _servo->setValue(0.0f, 2000); // Move servo to max position when button pressed
        }
        else if (btnState.isPressed && ledState.mode != "false")
        {
            _led->set(false);
            _servo->setValue(1.0f, 600); // Move servo to max position when button pressed
        }
    };

    // Getter implementations
    Led *Test2::getLed() { return _led; }
    Button *Test2::getButton() { return _button; }
    Servo *Test2::getServo() { return _servo; }

} // namespace composition
