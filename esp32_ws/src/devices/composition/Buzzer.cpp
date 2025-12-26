/**
 * @file Buzzer.cpp
 * @brief Buzzer implementation using DeviceBase and composition mixins
 */

#include "devices/composition/Buzzer.h"
#include "Logging.h"
#include <NonBlockingRtttl.h>
#include <ArduinoJson.h>

namespace composition
{

    Buzzer::Buzzer(const String &id)
        : DeviceBase(id, "buzzer")
    {
        // Fixed channel 0 for buzzer (required by NonBlockingRTTTL library)
        // https://github.com/end2endzone/NonBlockingRTTTL/blob/master/src/NonBlockingRtttl.cpp#L90C5-L99C6
        _ledcChannel = 0;
        LedcChannels::acquireSpecific(_ledcChannel);
    }

    Buzzer::~Buzzer()
    {
        if (_ledcChannel >= 0)
        {
            LedcChannels::release(_ledcChannel);
        }
    }

    void Buzzer::setup()
    {
        DeviceBase::setup();

        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured (pin = -1)", toString().c_str());
            return;
        }

        // Set the device name
        setName(_config.name);

        pinMode(_config.pin, OUTPUT);
        digitalWrite(_config.pin, LOW);

        _state.mode = "IDLE";
        _state.currentTune = "";
        noTone(_config.pin);

        MLOG_INFO("%s: Setup on pin %d", toString().c_str(), _config.pin);
    }

    void Buzzer::loop()
    {
        DeviceBase::loop();

        // Handle RTTTL tune playback
        if (rtttl::isPlaying())
        {
            rtttl::play();
        }
        else if (_state.mode == "TUNE")
        {
            // Tune was playing but has now finished
            _state.mode = "IDLE";
            _state.currentTune = "";

            notifyStateChanged();
        }

        // Check if we're currently playing a tone and if the duration has elapsed
        if (_state.mode == "TONE" && _state.toneDuration > 0)
        {
            unsigned long elapsed = millis() - _state.playStartTime;
            if (elapsed >= _state.toneDuration)
            {
                // Tone playback duration has finished
                _state.mode = "IDLE";
                _state.currentTune = "";
                _state.toneDuration = 0;

                notifyStateChanged();
            }
        }
    }

    std::vector<int> Buzzer::getPins() const
    {
        if (_config.pin == -1)
        {
            return {};
        }
        return {_config.pin};
    }

    bool Buzzer::tone(int frequency, int duration)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        MLOG_INFO("%s: Playing tone %dHz for %dms", toString().c_str(), frequency, duration);
        ::tone(_config.pin, frequency, duration);

        _state.mode = "TONE";
        _state.playStartTime = millis();
        _state.toneDuration = duration;
        _state.currentTune = "";

        notifyStateChanged();
        return true;
    }

    bool Buzzer::tune(const String &rtttlStr)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return false;
        }

        MLOG_INFO("%s: Playing RTTTL tune: %s", toString().c_str(), rtttlStr.c_str());

        rtttl::begin(_config.pin, rtttlStr.c_str());

        _state.currentTune = rtttlStr;
        _state.mode = "TUNE";
        _state.playStartTime = millis();

        notifyStateChanged();
        return true;
    }

    void Buzzer::addStateToJson(JsonDocument &doc)
    {
        doc["mode"] = _state.mode;
    }

    bool Buzzer::control(const String &action, JsonObject *args)
    {
        if (action == "tone")
        {
            if (!args)
            {
                MLOG_ERROR("%s: 'tone' action requires args", toString().c_str());
                return false;
            }

            if (!(*args)["frequency"].is<int>())
            {
                MLOG_ERROR("%s: Invalid 'tone' args - frequency missing", toString().c_str());
                return false;
            }

            if (!(*args)["duration"].is<int>())
            {
                MLOG_ERROR("%s: Invalid 'tone' args - duration missing", toString().c_str());
                return false;
            }

            const int frequency = (*args)["frequency"].as<int>();
            const int duration = (*args)["duration"].as<int>();

            // Validate frequency range
            if (frequency < 20 || frequency > 20000)
            {
                MLOG_ERROR("%s: Invalid frequency %dHz (range: 20-20000)", toString().c_str(), frequency);
                return false;
            }

            // Validate duration
            if (duration < 1 || duration > 10000)
            {
                MLOG_ERROR("%s: Invalid duration %dms (range: 1-10000)", toString().c_str(), duration);
                return false;
            }

            return tone(frequency, duration);
        }
        else if (action == "tune")
        {
            if (!args)
            {
                MLOG_ERROR("%s: 'tune' action requires args", toString().c_str());
                return false;
            }

            if (!(*args)["rtttl"].is<String>())
            {
                MLOG_ERROR("%s: Invalid 'tune' args - need rtttl string", toString().c_str());
                return false;
            }

            const String rtttlStr = (*args)["rtttl"].as<String>();
            if (rtttlStr.length() == 0)
            {
                MLOG_ERROR("%s: Empty RTTTL string", toString().c_str());
                return false;
            }

            return tune(rtttlStr);
        }
        else
        {
            MLOG_WARN("%s: Unknown control action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Buzzer::jsonToConfig(const JsonDocument &config)
    {
        if (config["pin"].is<int>())
        {
            _config.pin = config["pin"].as<int>();
        }
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
    }

    void Buzzer::configToJson(JsonDocument &doc)
    {
        doc["pin"] = _config.pin;
        doc["name"] = _config.name;
    }

} // namespace composition
