#include "devices/TestTaskDevice.h"
#include "devices/LedDevice.h"
#include "Logging.h"

TestTaskDevice::TestTaskDevice(const String &id, NotifyClients callback)
    : ControllableTaskDevice(id, "test-task", callback)
{
    // Create child devices
    _button = new ButtonDevice(id + "-btn", callback);
    _led = new LedDevice(id + "-led", callback);

    // Add as children
    addChild(_button);
    addChild(_led);
}

bool TestTaskDevice::setup(const JsonDocument &config)
{
    // Setup button
    JsonDocument btnConfig;
    btnConfig["pin"] = 19;
    btnConfig["name"] = "Test Button";
    btnConfig["debounceTimeInMs"] = 50;
    btnConfig["pinMode"] = "Floating";
    _button->setup(btnConfig);

    // Setup LED
    JsonDocument ledConfig;
    ledConfig["pin"] = 21;
    ledConfig["name"] = "Test LED";
    _led->setup(ledConfig);

    // Subscribe to button state changes
    _button->clearStateChangeCallbacks();
    _button->onStateChange([this](void *data)
                           {
                               if (_taskHandle)
                               {
                                   xTaskNotifyGive(_taskHandle);
                               } });

    // Call parent setup to start the task
    return SaveableTaskDevice::setup(config);
}

void TestTaskDevice::update()
{
    if (_button)
    {
        bool pressed = _button->isPressed();
        if (pressed)
        {
            MLOG_INFO("[%s]: Hello (from update)", toString().c_str());

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
    }
}

void TestTaskDevice::task()
{
    MLOG_INFO("%s: TestTaskDevice task started", toString().c_str());

    // Check at start
    this->update();

    while (true)
    {
        // Wait for notification (e.g. from button callback)
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            this->update();
        }
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