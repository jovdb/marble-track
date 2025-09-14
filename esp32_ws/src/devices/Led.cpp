#include "esp_log.h"
#include "devices/Led.h"

static const char *TAG = "Led";

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with pin, id, and name parameters.
 * @param pin GPIO pin number for the LED
 * @param id Unique identifier string for the LED
 * @param name Human-readable name string for the LED
 */
Led::Led(const String &id, const String &name)
    : Device(id, "led"), _mode(LedMode::OFF)
{
    _name = name;
}

/**
 * @brief Setup the LED hardware pin
 * @param pin GPIO pin number for the LED
 */
void Led::setup()
{
    Device::setup();

    if (_pin == -1) {
        ESP_LOGW(TAG, "Led [%s]: Pin not configured - call configurePin() first", _id.c_str());
        return;
    }

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    ESP_LOGI(TAG, "Led [%s]: Setup on pin %d", _id.c_str(), _pin);

    Led::blink(100, 300);
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 */
void Led::set(bool state)
{
    if (_pin == -1) {
        ESP_LOGW(TAG, "Led [%s]: Pin not configured - cannot set state", _id.c_str());
        return;
    }

    digitalWrite(_pin, state ? HIGH : LOW);
    _mode = state ? LedMode::ON : LedMode::OFF;

    // Notify state change for real-time updates
    notifyStateChange();
}

/**
 * @brief Toggle LED state
 */
void Led::toggle()
{
    set(_mode != LedMode::ON);
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
            ESP_LOGE(TAG, "Led [%s]: Invalid 'set' payload", _id.c_str());
            return false;
        }
        set((*args)["value"].as<bool>());
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
        ESP_LOGW(TAG, "Led [%s]: Unknown action: %s", _id.c_str(), action.c_str());
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

void Led::blink(unsigned long onTime, unsigned long offTime)
{
    _mode = LedMode::BLINKING;
    _blinkOnTime = onTime;
    _blinkOffTime = offTime;
    _lastToggleTime = millis() - 1000000;

    // Notify state change for real-time updates
    notifyStateChange();
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

/**
 * @brief Configure the LED pin
 * @param pin GPIO pin number for the LED
 */
void Led::configurePin(int pin)
{
    if (pin < 0) {
        ESP_LOGW(TAG, "Led [%s]: Invalid pin number %d", _id.c_str(), pin);
        return;
    }

    _pin = pin;
    ESP_LOGI(TAG, "Led [%s]: Configured with pin %d", _id.c_str(), _pin);
}

/**
 * @brief Get configuration as JSON
 * @return JsonObject containing the current configuration
 */
JsonObject Led::getConfig() const
{
    JsonDocument doc;
    JsonObject config = doc.to<JsonObject>();
    
    config["configured"] = (_pin != -1);
    if (_pin != -1) {
        config["pin"] = _pin;
    }
    config["name"] = _name;
    
    return config;
}

/**
 * @brief Set configuration from JSON
 * @param config Pointer to JSON object containing configuration
 */
void Led::setConfig(JsonObject *config)
{
    if (!config) {
        ESP_LOGW(TAG, "Led [%s]: Null config provided", _id.c_str());
        return;
    }
    
    // Set name if provided
    if ((*config)["name"].is<String>()) {
        _name = (*config)["name"].as<String>();
        ESP_LOGI(TAG, "Led [%s]: Name updated to '%s'", _id.c_str(), _name.c_str());
    }
    
    // Set pin if provided
    if ((*config)["pin"].is<int>()) {
        int pin = (*config)["pin"].as<int>();
        configurePin(pin);
        ESP_LOGI(TAG, "Led [%s]: Configured from JSON with pin %d", _id.c_str(), pin);
    }
}