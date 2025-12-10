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
    btnConfig["pinMode"] = "PullUp";
    _button->setup(btnConfig);
    
    // Setup LED
    JsonDocument ledConfig;
    ledConfig["pin"] = 21;
    ledConfig["name"] = "Test LED";
    _led->setup(ledConfig);
    
    // Call parent setup to start the task
    return SaveableTaskDevice::setup(config);
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