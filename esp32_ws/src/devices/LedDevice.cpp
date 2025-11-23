#include "devices/LedDevice.h"
#include "Logging.h"

LedDevice::LedDevice(const String &id) : ControllableTaskDevice(id, "led")
{
}

void LedDevice::updateConfig(const JsonDocument &config)
{
    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }
    if (config["name"].is<String>())
    {
        _name = config["name"].as<String>();
    }

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

JsonDocument LedDevice::getConfig() const
{
    JsonDocument doc = SaveableTaskDevice::getConfig();
    doc["pin"] = _pin;
    doc["name"] = _name;
    return doc;
}

void LedDevice::addToState(JsonDocument &doc)
{
    if (_targetMode == Mode::BLINKING)
    {
        doc["mode"] = "blinking";
    }
    else
    {
        doc["mode"] = _isOn ? "on" : "off";
    }
}

bool LedDevice::control(const String &action, JsonObject *args)
{
    if (action == "set")
    {
        if (args && (*args)["value"].is<bool>())
        {
            set((*args)["value"].as<bool>());
            return true;
        }
    }
    else if (action == "blink")
    {
        unsigned long onTime = 500;
        unsigned long offTime = 500;
        if (args)
        {
            if ((*args)["onTime"].is<unsigned long>())
                onTime = (*args)["onTime"].as<unsigned long>();
            if ((*args)["offTime"].is<unsigned long>())
                offTime = (*args)["offTime"].as<unsigned long>();
        }
        blink(onTime, offTime);
        return true;
    }
    return false;
}

void LedDevice::set(bool state)
{
    _targetMode = state ? Mode::ON : Mode::OFF;
    _targetState = state;

    if (_taskHandle)
    {
        xTaskNotifyGive(_taskHandle);
    }
}

void LedDevice::blink(unsigned long onTime, unsigned long offTime)
{
    _targetMode = Mode::BLINKING;
    _targetBlinkOnTime = onTime;
    _targetBlinkOffTime = offTime;

    if (_taskHandle)
    {
        xTaskNotifyGive(_taskHandle);
    }
}

void LedDevice::task()
{
    while (true)
    {
        // Read volatile targets into local variables
        Mode currentMode = _targetMode;

        if (currentMode == Mode::BLINKING)
        {
            // Determine wait time based on current state
            unsigned long waitTime = _isOn ? _targetBlinkOnTime : _targetBlinkOffTime;

            // Wait for notification OR timeout (blink toggle)
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitTime)) > 0)
            {
                // Received a notification (e.g. set() or blink() called)
                // Loop will restart and pick up new targets
                continue;
            }

            // Timeout occurred: Toggle LED
            _isOn = !_isOn;
            digitalWrite(_pin, _isOn ? HIGH : LOW);
        }
        else
        {
            // Static state (ON or OFF)
            bool targetState = _targetState;

            // Apply state
            digitalWrite(_pin, targetState ? HIGH : LOW);
            _isOn = targetState;

            // Sleep indefinitely until notified
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}
