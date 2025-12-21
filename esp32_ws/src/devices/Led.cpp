#include "Logging.h"
#include "devices/Led.h"

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with just the ID parameter.
 * @param id Unique identifier string for the LED
 */
Led::Led(const String &id, NotifyClients callback)
    : Device(id, "led", callback), _mode(LedMode::OFF), _initialMode(LedMode::OFF)
{
    // Name can be set later using setName()
}

/**
 * @brief Setup the LED hardware pin
 * @param pin GPIO pin number for the LED
 */
void Led::setup()
{
    Device::setup();

    if (_pin != -1)
    {
        pinMode(_pin, OUTPUT);
    }
    else
    {
        MLOG_WARN("Led [%s]: Pin not configured", _id.c_str());
    }

    applyInitialState();
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 * @return true if set successfully, false if not configured
 */
bool Led::set(bool state)
{
    if (_pin == -1)
        return false;

    // prevent notify in loop if not changed
    if (_mode == LedMode::OFF && !state)
        return true;
    if (_mode == LedMode::ON && state)
        return true;

    digitalWrite(_pin, state ? HIGH : LOW);
    _mode = state ? LedMode::ON : LedMode::OFF;
    _targetMode = _mode;
    _targetState = state;
    _isOn = state;

    // Notify task to update immediately
    if (_taskHandle != nullptr)
    {
        xTaskNotifyGive(_taskHandle);
    }

    // Notify state change for real-time updates
    notifyStateChange();
    return true;
}

/**
 * @brief Non-blocking blink
 * @param onTime time in ms LED is on
 * @param offTime time in ms LED is off
 * @return true if blink started, false if not configured
 */
bool Led::blink(unsigned long onTime, unsigned long offTime)
{
    if (_pin == -1)
    {
        MLOG_WARN("Led [%s]: Pin not configured", _id.c_str());
        return false;
    }

    _mode = LedMode::BLINKING;
    _targetMode = LedMode::BLINKING;
    _blinkOnTime = onTime;
    _blinkOffTime = offTime;
    _targetBlinkOnTime = onTime;
    _targetBlinkOffTime = offTime;
    _lastToggleTime = millis() - 1000000;

    // Notify task to update immediately
    if (_taskHandle != nullptr)
    {
        xTaskNotifyGive(_taskHandle);
    }

    // Notify state change for real-time updates
    notifyStateChange();
    return true;
}

/**
 * @brief Dynamic control function for LED operations
 * @param action The action to perform (e.g., "set")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Led::control(const String &action, JsonObject *args)
{
    if (action == "set")
    {
        if (!args || !(*args)["value"].is<bool>())
        {
            MLOG_ERROR("Led [%s]: Invalid 'set' payload", _id.c_str());
            return false;
        }
        const bool on = (*args)["value"].as<bool>();
        set(on);
        return true;
    }
    else if (action == "blink")
    {
        unsigned long onTime = 500;
        unsigned long offTime = 500;
        if (args)
        {
            if ((*args)["onTime"].is<unsigned long>())
            {
                const unsigned long extractedOnTime = (*args)["onTime"].as<unsigned long>();
                onTime = extractedOnTime;
            }
            if ((*args)["offTime"].is<unsigned long>())
            {
                const unsigned long extractedOffTime = (*args)["offTime"].as<unsigned long>();
                offTime = extractedOffTime;
            }
        }
        blink(onTime, offTime);
        return true;
    }
    else
    {
        MLOG_WARN("Led [%s]: Unknown action: %s", _id.c_str(), action.c_str());
        return false;
    }
}

/**
 * @brief Get current state of the LED
 * @return String containing JSON representation of the current state
 */
String Led::getState()
{

    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    switch (_mode)
    {
    case LedMode::ON:
        doc["mode"] = "ON";
        break;
    case LedMode::OFF:
        doc["mode"] = "OFF";
        break;
    case LedMode::BLINKING:
        doc["mode"] = "BLINKING";
        break;
    }

    String result;
    serializeJson(doc, result);
    return result;
}

void Led::task()
{
    while (true)
    {
        if (_pin == -1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Read atomic targets into local variables
        LedMode currentMode = _targetMode;
        bool targetState = _targetState;

        if (currentMode == LedMode::BLINKING)
        {
            // Determine wait time based on current state
            unsigned long waitTime = _isOn ? _targetBlinkOnTime.load() : _targetBlinkOffTime.load();

            // Wait for notification OR timeout (blink toggle)
            // ulTaskNotifyTake(pdTRUE, ...) clears the notification value on exit
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitTime)) > 0)
            {
                // Received a notification (e.g. set() or blink() called)
                // Loop will restart and pick up new _targetMode/_targetState
                continue;
            }

            // Timeout occurred: Toggle LED
            _isOn = !_isOn;
            digitalWrite(_pin, _isOn ? HIGH : LOW);
        }
        else
        {
            // Static state (ON or OFF)
            digitalWrite(_pin, targetState ? HIGH : LOW);
            _isOn = targetState;

            // Sleep indefinitely until notified
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

std::vector<int> Led::getPins() const
{
    if (_pin == -1)
    {
        return {};
    }

    std::vector<int> pins;
    pins.push_back(_pin);
    return pins;
}

/**
 * @brief Get configuration as JSON
 * @return JsonObject containing the current configuration
 */
String Led::getConfig() const
{
    JsonDocument config;
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;
    config["initialState"] = modeToString(_initialMode);

    String message;
    serializeJson(config, message);

    return message;
}

/**
 * @brief Set configuration from JSON
 * @param config Pointer to JSON object containing configuration
 */
void Led::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Led [%s]: Null config provided", _id.c_str());
        return;
    }

    // Set name if provided
    if ((*config)["name"].is<String>())
    {
        const String name = (*config)["name"].as<String>();
        _name = name;
        //  ESP_LOGI(TAG, "Led [%s]: Name updated to '%s'", _id.c_str(), _name.c_str());
    }

    // Set pin if provided
    if ((*config)["pin"].is<int>())
    {
        const int pin = (*config)["pin"].as<int>();
        _pin = pin;
    }

    if ((*config)["initialState"].is<String>())
    {
        const String initial = (*config)["initialState"].as<String>();
        _initialMode = modeFromString(initial);
    }

    // Apply pin mode and initial behaviour
    setup();
}

void Led::applyInitialState()
{
    switch (_initialMode)
    {
    case LedMode::ON:
        set(true);
        break;
    case LedMode::OFF:
        set(false);
        break;
    case LedMode::BLINKING:
        blink(_blinkOnTime, _blinkOffTime);
        break;
    }
}

Led::LedMode Led::modeFromString(const String &value) const
{
    if (value.equalsIgnoreCase("ON"))
    {
        return LedMode::ON;
    }
    if (value.equalsIgnoreCase("BLINKING"))
    {
        return LedMode::BLINKING;
    }
    return LedMode::OFF;
}

String Led::modeToString(LedMode mode) const
{
    switch (mode)
    {
    case LedMode::ON:
        return "ON";
    case LedMode::BLINKING:
        return "BLINKING";
    default:
        return "OFF";
    }
}