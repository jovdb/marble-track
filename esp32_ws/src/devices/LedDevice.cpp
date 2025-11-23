#include "devices/LedDevice.h"
#include "Logging.h"

LedDevice::LedDevice(const String &id) : TaskDevice(id, "led")
{
}

bool LedDevice::setup(uint8_t pin, const String &name)
{
    _pin = pin;
    _name = name;

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);

    return TaskDevice::setup(_name);
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
