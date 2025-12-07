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
        _name = "LED Device";  // Default name
    }

    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void LedDevice::addConfigToJson(JsonDocument &doc) const
{
    doc["pin"] = _pin;
    doc["name"] = _name;
}

void LedDevice::addStateToJson(JsonDocument &doc)
{
    if (_targetMode == Mode::BLINKING)
    {
        doc["mode"] = "BLINKING";
        doc["onTime"] = _targetBlinkOnTime;
        doc["offTime"] = _targetBlinkOffTime;
    }
    else if (_targetMode == Mode::ON || _targetMode == Mode::OFF)
    {
        doc["mode"] = _targetState ? "ON" : "OFF";
    }
    else
    {
        MLOG_ERROR("[%s]: Unknown _targetMode", toString().c_str());
    }
}

bool LedDevice::control(const String &action, JsonObject *args)
{
    if (action == "set")
    {
        if (args && (*args)["value"].is<bool>())
        {
            bool state = (*args)["value"].as<bool>();
            set(state);
            MLOG_INFO("[%s]: Set to %s", toString().c_str(), state ? "ON" : "OFF");
            return true;
        }
        else
        {
            MLOG_WARN("[%s]: Invalid args for 'set' action", toString().c_str());
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
                if (onTime == 0 || onTime > 10000) onTime = DEFAULT_BLINK_TIME;  // Validate range
            }
            if ((*args)["offTime"].is<unsigned long>())
            {
                offTime = (*args)["offTime"].as<unsigned long>();
                if (offTime == 0 || offTime > 10000) offTime = DEFAULT_BLINK_TIME;  // Validate range
            }
        }
        blink(onTime, offTime);
        MLOG_INFO("[%s]: Blinking with onTime=%lu, offTime=%lu", toString().c_str(), onTime, offTime);
        return true;
    }
    else
    {
        MLOG_WARN("[%s]: Unknown action '%s'", toString().c_str(), action.c_str());
    }
    return false;
}

std::vector<int> LedDevice::getPins() const
{
    return {_pin};
}

void LedDevice::set(bool state)
{
    _targetMode = state ? Mode::ON : Mode::OFF;
    _targetState = state;

    notifyState(true);

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

    notifyState(true);

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
            // Reset _isOn if switching to blinking
            if (_targetMode != Mode::BLINKING) _isOn = false;

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
