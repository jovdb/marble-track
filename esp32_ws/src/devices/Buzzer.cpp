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
#include <Arduino.h> // Needed for tone, noTone, delay
#include "Logging.h"

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
Buzzer::Buzzer(const String &id, const String &name)
    : Device(id, "buzzer")
{
    _name = name;
    _pin = -1;
    _isPlaying = false;
    _mode = BuzzerMode::IDLE;
    _playStartTime = 0;
    _toneDuration = 0;
}

/**
 * @brief Play a startup tone sequence
 */
void Buzzer::startupTone()
{
    if (_pin < 0)
    {
        MLOG_WARN("Buzzer [%s]: Pin not configured, skipping startup tone", _id.c_str());
        return;
    }

    for (int frequency = 200; frequency <= 1000; frequency += 200)
    {
        this->tone(frequency, 80); // Play tone for 80ms using Buzzer::tone
        delay(20);                 // Small delay to separate tones
    }
    noTone(_pin); // Ensure tone is off before the next loop
}

/**
 * @brief Setup function to initialize the buzzer
 * Must be called in setup() before using the buzzer
 */
void Buzzer::setup()
{
    if (_pin < 0)
    {
        MLOG_WARN("Buzzer [%s]: Pin not configured, defaulting to pin 2", _id.c_str());
        _pin = 2;
    }

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _isPlaying = false;
    _mode = BuzzerMode::IDLE;
    _currentTune.clear();
    noTone(_pin);
}

/**
 * @brief Loop function for continuous buzzer operations
 *
 * Handles timing for tone and tune playback without blocking.
 * Should be called repeatedly in the main loop.
 */
void Buzzer::loop()
{
    // Handle RTTTL tune playback
    if (rtttl::isPlaying())
    {
        rtttl::play();
    }
    else if (_mode == BuzzerMode::TUNE && _isPlaying)
    {
        // Tune was playing but has now finished
        _isPlaying = false;
        _mode = BuzzerMode::IDLE;
        _currentTune = "";

        notifyStateChange();
    }

    // Check if we're currently playing a tone and if the duration has elapsed
    if (_isPlaying && _mode == BuzzerMode::TONE && _toneDuration > 0)
    {
        unsigned long elapsed = millis() - _playStartTime;
        if (elapsed >= _toneDuration)
        {
            // Tone playback duration has finished
            _isPlaying = false;
            _mode = BuzzerMode::IDLE;
            _currentTune = "";
            _toneDuration = 0;

            // TODO: Stop hardware tone generation here
            // ::noTone(_pin);

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
    if (_pin < 0)
    {
        MLOG_WARN("Buzzer [%s]: Pin not configured, ignoring tone command", _id.c_str());
        return;
    }

    MLOG_INFO("Buzzer [%s]: Playing tone %dHz for %dms", _id.c_str(), frequency, duration);
    ::tone(_pin, frequency, duration);

    _isPlaying = true;
    _mode = BuzzerMode::TONE;
    _playStartTime = millis();
    _toneDuration = duration;
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
    if (_pin < 0)
    {
        MLOG_WARN("Buzzer [%s]: Pin not configured, ignoring tune command", _id.c_str());
        return;
    }

    MLOG_INFO("Buzzer [%s]: Playing RTTTL tune: %s", _id.c_str(), rtttl.c_str());

    rtttl::begin(_pin, rtttl.c_str());

    _currentTune = rtttl;
    _isPlaying = true;
    _mode = BuzzerMode::TUNE;
    _playStartTime = millis();
    // Note: _toneDuration is not set for tunes - RTTTL library manages tune duration
    notifyStateChange();
}

/**
 * @brief Dynamic control function for buzzer operations
 * @param action The action to perform (e.g., "tone", "tune")
 * @param payload Pointer to JSON object containing action parameters (can be nullptr)
 * @return true if action was successful, false otherwise
 */
bool Buzzer::control(const String &action, JsonObject *args)
{
    if (action == "tone")
    {
        if (!args)
        {
            MLOG_ERROR("Buzzer [%s]: args missing", _id.c_str());
            return false;
        }

        if (!(*args)["frequency"].is<int>())
        {
            MLOG_ERROR("Buzzer [%s]: Invalid 'tone' args - frequency missing", _id.c_str());
            return false;
        }

        if (!(*args)["duration"].is<int>())
        {
            MLOG_ERROR("Buzzer [%s]: Invalid 'tone' args - duration missing", _id.c_str());
            return false;
        }

        int frequency = (*args)["frequency"].as<int>();
        int duration = (*args)["duration"].as<int>();

        // Validate frequency range
        if (frequency < 20 || frequency > 20000)
        {
            MLOG_ERROR("Buzzer [%s]: Invalid frequency %dHz (range: 20-20000)", _id.c_str(), frequency);
            return false;
        }

        // Validate duration
        if (duration < 1 || duration > 10000)
        {
            MLOG_ERROR("Buzzer [%s]: Invalid duration %dms (range: 1-10000)", _id.c_str(), duration);
            return false;
        }

        tone(frequency, duration);
        return true;
    }
    else if (action == "tune")
    {
        if (!args)
        {
            MLOG_ERROR("Buzzer [%s]: Invalid 'tune' - args missing", _id.c_str());
            return false;
        }

        if (!(*args)["rtttl"].is<String>())
        {
            MLOG_ERROR("Buzzer [%s]: Invalid 'tune' args - need rtttl string", _id.c_str());
            return false;
        }

        String rtttl = (*args)["rtttl"].as<String>();
        if (rtttl.length() == 0)
        {
            MLOG_ERROR("Buzzer [%s]: Empty RTTTL string", _id.c_str());
            return false;
        }

        tune(rtttl);
        return true;
    }
    else
    {
        MLOG_WARN("Buzzer [%s]: Unknown action: %s", _id.c_str(), action.c_str());
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

    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }

    // Convert mode enum to string
    String modeStr;
    switch (_mode)
    {
    case BuzzerMode::IDLE:
        modeStr = "IDLE";
        break;
    case BuzzerMode::TONE:
        modeStr = "TONE";
        break;
    case BuzzerMode::TUNE:
        modeStr = "TUNE";
        break;
    default:
        modeStr = "UNKNOWN";
        break;
    }
    doc["mode"] = modeStr;

    doc["playing"] = _isPlaying;

    String result;
    serializeJson(doc, result);
    return result;
}

String Buzzer::getConfig() const
{
    DynamicJsonDocument config(256);
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;

    String message;
    serializeJson(config, message);
    return message;
}

void Buzzer::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Buzzer [%s]: Null config provided", _id.c_str());
        return;
    }

    JsonVariant nameVariant = (*config)["name"];
    if (!nameVariant.isNull())
    {
        _name = nameVariant.as<String>();
    }

    JsonVariant pinVariant = (*config)["pin"];
    if (!pinVariant.isNull())
    {
        _pin = pinVariant.as<int>();
    }

    setup();
}

std::vector<int> Buzzer::getPins() const
{
    if (_pin < 0)
    {
        return {};
    }
    return {_pin};
}
