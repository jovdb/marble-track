#include "devices/TestTaskDevice.h"
#include "devices/LedDevice.h"
#include "Logging.h"

TestTaskDevice::TestTaskDevice(const String &id, ButtonDevice *button, LedDevice *led, NotifyClients callback)
    : ControllableTaskDevice(id, "test-task", callback), _button(button), _led(led)
{
}

void TestTaskDevice::task()
{
    MLOG_INFO("%s: TestTaskDevice task started", toString().c_str());

    // Initialize last pressed state
    if (_button)
    {
        _lastPressed = _button->isPressed();
    }

    while (true)
    {
        if (!_button)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        bool currentPressed = _button->isPressed();
        if (currentPressed && !_lastPressed)
        {
            MLOG_INFO("[%s]: Hello", toString().c_str());
            
            // Toggle LED blinking
            if (_led)
            {
                if (_led->mode() == LedDevice::Mode::BLINKING)
                {
                    MLOG_INFO("[%s]: Turn LED OFF", toString().c_str());
                    _led->set(false); // Turn off LED
                }
                else
                {
                    _led->blink(200, 200); // Start blinking
                    MLOG_INFO("[%s]: LED started blinking", toString().c_str());
                }
            }
        }
        _lastPressed = currentPressed;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TestTaskDevice::getConfigFromJson(const JsonDocument &config)
{
    // No configuration needed
}

void TestTaskDevice::addConfigToJson(JsonDocument &doc) const
{
    // No configuration to add
}