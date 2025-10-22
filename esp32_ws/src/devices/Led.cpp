#include "Logging.h"
#include "devices/Led.h"

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with just the ID parameter.
 * @param id Unique identifier string for the LED
 */
Led::Led(const String &id)
    : Device(id, "led"), _mode(LedMode::OFF), _initialMode(LedMode::OFF)
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

    if (!_didSetup)
    {
        applyInitialState();
        _didSetup = true;
    }
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 * @return true if set successfully, false if not configured
 */
bool Led::set(bool state)
{
    if (_pin == -1)
    {
        MLOG_WARN("Led [%s]: Pin not configured", _id.c_str());
        return false;
    }

    digitalWrite(_pin, state ? HIGH : LOW);
    _mode = state ? LedMode::ON : LedMode::OFF;
    _isOn = state;

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
    _blinkOnTime = onTime;
    _blinkOffTime = offTime;
    _lastToggleTime = millis() - 1000000;

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
        boolean on = (*args)["value"].as<bool>();
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
                onTime = (*args)["onTime"].as<unsigned long>();
            if ((*args)["offTime"].is<unsigned long>())
                offTime = (*args)["offTime"].as<unsigned long>();
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

void Led::loop()
{
    Device::loop();

    if (_mode == LedMode::BLINKING && _pin != -1)
    {
        unsigned long now = millis();
        if (_isOn)
        {
            if (now - _lastToggleTime >= _blinkOnTime)
            {
                digitalWrite(_pin, LOW);
                _isOn = false;
                _lastToggleTime = now;
            }
        }
        else
        {
            if (now - _lastToggleTime >= _blinkOffTime)
            {
                digitalWrite(_pin, HIGH);
                _isOn = true;
                _lastToggleTime = now;
            }
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
        _name = (*config)["name"].as<String>();
        //  ESP_LOGI(TAG, "Led [%s]: Name updated to '%s'", _id.c_str(), _name.c_str());
    }

    // Set pin if provided
    if ((*config)["pin"].is<int>())
    {
        int pin = (*config)["pin"].as<int>();
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