#include "esp_log.h"

static const char *TAG = "Led";
/**
 * @file Led.cpp
 * @brief Implementation of LED control class
 *
 * This file contains the implementation of the Led class methods
 * for controlling LEDs in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Led.h"

/**
 * @brief Constructor for Led class
 *
 * Initializes the Led object with pin, id, and name parameters.
 * @param pin GPIO pin number for the LED
 * @param id Unique identifier string for the LED
 * @param name Human-readable name string for the LED
 */
Led::Led(int pin, const String &id, const String &name)
    : Device(id, name, "led"), _pin(pin), _mode("OFF")
{
    // Initialize the pin as output and set initial state
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    ESP_LOGI(TAG, "Led [%s]: Initialized on pin %d", _id.c_str(), _pin);
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
bool Led::control(const String &action, JsonObject *payload)
{
    // Simple action mapping
    if (action == "on")
    {
        set(true);
    }
    else if (action == "off")
    {
        set(false);
    }
    else if (action == "toggle")
    {
        set(!_mode.equals("ON"));
    }
    else if (action == "set")
    {
        if (!payload || !(*payload)["state"].is<bool>())
        {
            ESP_LOGE(TAG, "Led [%s]: Invalid 'set' payload", _id.c_str());
            return false;
        }
        set((*payload)["state"].as<bool>());
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
    for (JsonPair kv : baseDoc.as<JsonObject>()) {
        doc[kv.key()] = kv.value();
    }
    doc["mode"] = _mode;

    String result;
    serializeJson(doc, result);
    return result;
}
