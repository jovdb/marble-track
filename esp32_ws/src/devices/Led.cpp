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
    : Device(id, name, "led"), _mode("OFF")
{
}

/**
 * @brief Setup the LED hardware pin
 * @param pin GPIO pin number for the LED
 */
void Led::setup()
{
    Device::setup();

    _pin = 1;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    ESP_LOGI(TAG, "Led [%s]: Setup on pin %d", _id.c_str(), _pin);
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 */
void Led::set(bool state)
{
    digitalWrite(_pin, state ? HIGH : LOW);
    _mode = state ? "ON" : "OFF";

    // Notify state change for real-time updates
    notifyStateChange();
}

/**
 * @brief Toggle LED state
 */
void Led::toggle()
{
    set(!_mode.equals("ON"));
}

/**
 * @brief Dynamic control function for LED operations
 * @param action The action to perform (e.g., "set")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Led::control(const String &action, JsonObject *args)
{
    // Simple action mapping
    if (action == "set")
    {
        if (!args || !(*args)["value"].is<bool>())
        {
            ESP_LOGE(TAG, "Led [%s]: Invalid 'set' payload", _id.c_str());
            return false;
        }
        set((*args)["value"].as<bool>());
    }
    else
    {
        ESP_LOGW(TAG, "Led [%s]: Unknown action: %s", _id.c_str(), action.c_str());
        return false;
    }

    return true;
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
    doc["mode"] = _mode;

    String result;
    serializeJson(doc, result);
    return result;
}
