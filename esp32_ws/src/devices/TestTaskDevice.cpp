#include "devices/TestTaskDevice.h"
#include "Logging.h"

TestTaskDevice::TestTaskDevice(const String &id, ButtonDevice *button, NotifyClients callback)
    : ControllableTaskDevice(id, "test-task", callback), _button(button)
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