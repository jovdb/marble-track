/**
 * @file Buzzer.cpp
 * @brief Implementation of Buzzer control class
 *
 * This file contains the implementation of the Buzzer class methods
 * for controlling buzzers and playing tones/tunes in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "devices/Buzzer.h"
#include <NonBlockingRtttl.h>

/**
 * @brief Constructor for Buzzer class
 *
 * Initializes the Buzzer object with pin, id, and name parameters.
 * @param pin GPIO pin number for the buzzer
 * @param id Unique identifier string for the buzzer
 * @param name Human-readable name string for the buzzer
 */
Buzzer::Buzzer(int pin, const String &id, const String &name)
    : _pin(pin), _id(id), _name(name), _isPlaying(false), _playStartTime(0), _playDuration(0)
{
    Serial.println("Buzzer [" + _id + "]: Created on pin " + String(_pin));
}

/**
 * @brief Setup function to initialize the buzzer
 * Must be called in setup() before using the buzzer
 */
void Buzzer::setup()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    Serial.println("Buzzer [" + _id + "]: Setup complete on pin " + String(_pin));
}

/**
 * @brief Loop function for continuous buzzer operations
 *
 * Handles timing for tone and tune playback without blocking.
 * Should be called repeatedly in the main loop.
 */
void Buzzer::loop()
{

    if (rtttl::isPlaying())
    {
        rtttl::play();
    }

    // Check if we're currently playing and if the duration has elapsed
    if (_isPlaying && _playDuration > 0)
    {
        unsigned long elapsed = millis() - _playStartTime;
        if (elapsed >= _playDuration)
        {
            // Playback duration has finished
            _isPlaying = false;
            _currentTune = "";
            _playDuration = 0;

            // TODO: Stop hardware tone generation here
            // ::noTone(_pin);

            Serial.println("Buzzer [" + _id + "]: Playback finished");
            notifyStateChange();
        }
    }
}

/**
 * @brief Play a tone with specified frequency and duration
 * @param frequency Frequency in Hz (20-20000)
 * @param duration Duration in milliseconds
 */
void Buzzer::tone(int frequency, int duration)
{
    Serial.println("Buzzer [" + _id + "]: Playing tone " + String(frequency) + "Hz for " + String(duration) + "ms");
    ::tone(_pin, frequency, duration);

    _isPlaying = true;
    _playStartTime = millis();
    _playDuration = duration;
    _currentTune = "";
    notifyStateChange();
}

/**
 * @brief Play a tune from RTTTL string
 * @param rtttl RTTTL (Ring Tone Text Transfer Language) string
 * @note RTTTL format: "name:settings:notes"
 */
void Buzzer::tune(const String &rtttl)
{
    Serial.println("Buzzer [" + _id + "]: Playing RTTTL tune: " + rtttl);

    rtttl::begin(_pin, rtttl.c_str());

    _currentTune = rtttl;
    _isPlaying = true;
    _playStartTime = millis();
    _playDuration = 3000; // Default tune duration (3 seconds) - should be calculated from RTTTL
    notifyStateChange();
}

/**
 * @brief Dynamic control function for buzzer operations
 * @param action The action to perform (e.g., "tone", "tune")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Buzzer::control(const String &action, JsonObject *payload)
{
    if (action == "tone")
    {
        if (!payload || !(*payload)["frequency"].is<int>() || !(*payload)["duration"].is<int>())
        {
            Serial.println("Buzzer [" + _id + "]: Invalid 'tone' payload - need frequency and duration");
            return false;
        }

        int frequency = (*payload)["frequency"].as<int>();
        int duration = (*payload)["duration"].as<int>();

        // Validate frequency range
        if (frequency < 20 || frequency > 20000)
        {
            Serial.println("Buzzer [" + _id + "]: Invalid frequency " + String(frequency) + "Hz (range: 20-20000)");
            return false;
        }

        // Validate duration
        if (duration < 1 || duration > 10000)
        {
            Serial.println("Buzzer [" + _id + "]: Invalid duration " + String(duration) + "ms (range: 1-10000)");
            return false;
        }

        tone(frequency, duration);
        return true;
    }
    else if (action == "tune")
    {
        if (!payload || !(*payload)["rtttl"].is<String>())
        {
            Serial.println("Buzzer [" + _id + "]: Invalid 'tune' payload - need rtttl string");
            return false;
        }

        String rtttl = (*payload)["rtttl"].as<String>();
        if (rtttl.length() == 0)
        {
            Serial.println("Buzzer [" + _id + "]: Empty RTTTL string");
            return false;
        }

        tune(rtttl);
        return true;
    }
    else
    {
        Serial.println("Buzzer [" + _id + "]: Unknown action: " + action);
        return false;
    }
}

/**
 * @brief Get current state of the buzzer
 * @return String containing JSON representation of the current state
 */
String Buzzer::getState()
{
    JsonDocument doc;
    doc["playing"] = _isPlaying;
    doc["currentTune"] = _currentTune;
    doc["pin"] = _pin;
    doc["name"] = _name;
    doc["type"] = _type;

    String result;
    serializeJson(doc, result);
    return result;
}
