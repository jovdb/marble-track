#include "devices/LedDevice.h"
#include "Logging.h"

LedDevice::LedDevice(const String &id, NotifyClients callback) : ControllableTaskDevice(id, "led", callback)
{
}

void LedDevice::getConfigFromJson(const JsonDocument &config)
{
    if (config["name"].is<String>())
    {
        _name = config["name"].as<String>();
    }
    else
    {
        _name = "LED Device"; // Default name
    }

    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }

    if (_pin >= 0)
    {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        _isOn = false;
    }
    else
    {
        MLOG_WARN("%s: No valid pin configured", toString().c_str());
    }
}

void LedDevice::addConfigToJson(JsonDocument &doc) const
{
    doc["pin"] = _pin;
    doc["name"] = _name;
}

void LedDevice::addStateToJson(JsonDocument &doc)
{
    if (_desiredMode == Mode::BLINKING)
    {
        doc["mode"] = "BLINKING";
        doc["onTime"] = _blinkOnDurationMs;
        doc["offTime"] = _blinkOffDurationMs;
    }
    else if (_desiredMode == Mode::ON || _desiredMode == Mode::OFF)
    {
        doc["mode"] = _desiredState ? "ON" : "OFF";
    }
    else
    {
        MLOG_ERROR("%s: Unknown _desiredMode", toString().c_str());
    }
}

bool LedDevice::control(const String &action, JsonObject *args)
{
    if (_pin < 0)
    {
        MLOG_WARN("%s: Cannot control LED - no pin configured", toString().c_str());
        return false;
    }

    if (action == "set")
    {
        if (args && (*args)["value"].is<bool>())
        {
            bool state = (*args)["value"].as<bool>();
            set(state);
            MLOG_INFO("%s: Set to %s", toString().c_str(), state ? "ON" : "OFF");
            return true;
        }
        else
        {
            MLOG_WARN("%s: Invalid args for 'set' action", toString().c_str());
        }
    }
    else if (action == "blink")
    {
        unsigned long onTime = DEFAULT_BLINK_TIME;
        unsigned long offTime = DEFAULT_BLINK_TIME;
        if (args)
        {
            if ((*args)["onTime"].is<unsigned long>())
            {
                onTime = (*args)["onTime"].as<unsigned long>();
                if (onTime == 0 || onTime > 10000)
                    onTime = DEFAULT_BLINK_TIME; // Validate range
            }
            if ((*args)["offTime"].is<unsigned long>())
            {
                offTime = (*args)["offTime"].as<unsigned long>();
                if (offTime == 0 || offTime > 10000)
                    offTime = DEFAULT_BLINK_TIME; // Validate range
            }
        }
        blink(onTime, offTime);
        MLOG_INFO("%s: Blinking with onTime=%lu, offTime=%lu", toString().c_str(), onTime, offTime);
        return true;
    }
    else
    {
        MLOG_WARN("%s: Unknown action '%s'", toString().c_str(), action.c_str());
    }
    return false;
}

std::vector<int> LedDevice::getPins() const
{
    return {_pin};
}

void LedDevice::set(bool state)
{
    _desiredMode = state ? Mode::ON : Mode::OFF;
    _desiredState = state;

    notifyState(true);

    if (_taskHandle)
    {
        xTaskNotifyGive(_taskHandle);
    }
}

void LedDevice::blink(unsigned long onTime, unsigned long offTime)
{
    _desiredMode = Mode::BLINKING;
    _blinkOnDurationMs = onTime;
    _blinkOffDurationMs = offTime;

    notifyState(true);

    if (_taskHandle)
    {
        xTaskNotifyGive(_taskHandle);
    }
}

void LedDevice::task()
{
    if (_pin < 0)
    {
        MLOG_ERROR("%s: No valid pin configured, task exiting", toString().c_str());
        return;
    }

    while (true)
    {
        // Read volatile desired values into local variables
        // Take snapshots of volatile variables
        Mode snapshotMode = _desiredMode;

        if (snapshotMode == Mode::BLINKING)
        {
            // Determine wait time based on current state
            unsigned long waitTime = _isOn ? _blinkOnDurationMs : _blinkOffDurationMs;

            // Wait for notification OR timeout (blink toggle)
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitTime)) > 0)
            {
                // Received a notification (e.g. set() or blink() called)
                // Loop will restart and pick up new desired values
                continue;
            }

            // Timeout occurred: Toggle LED
            _isOn = !_isOn;
            digitalWrite(_pin, _isOn ? HIGH : LOW);
        }
        else
        {
            // Static state (ON or OFF)
            bool snapshotState = _desiredState;

            // Apply state only if it changed
            if (_isOn != snapshotState)
            {
                digitalWrite(_pin, snapshotState ? HIGH : LOW);
                _isOn = snapshotState;
            }

            // Sleep indefinitely until notified
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}
