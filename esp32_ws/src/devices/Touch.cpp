#include "devices/Touch.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    namespace
    {
        bool isValidTouchPin(int pin)
        {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
            return pin >= 1 && pin <= 14;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
            return pin >= 1 && pin <= 14;
#elif defined(CONFIG_IDF_TARGET_ESP32)
            const int validPins[] = {0, 2, 4, 12, 13, 14, 15, 27, 32, 33};
            for (int validPin : validPins)
            {
                if (pin == validPin)
                {
                    return true;
                }
            }
            return false;
#else
            return false;
#endif
        }
    }

    Touch::Touch(const String &id)
        : Device(id, "touch")
    {
    }

    Touch::~Touch()
    {
    }

    void Touch::setup()
    {
        Device::setup();

        setName(_config.name);

        _hasCandidate = false;
        _candidateTouched = false;
        _candidateSince = 0;
        _isSimulated = false;
        _simulatedTouched = false;

        _state.value = 0;
        _state.touched = false;
        _state.isTouchedChanged = false;

        if (_config.pin < 0)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return;
        }

        if (!isValidTouchPin(_config.pin))
        {
            MLOG_WARN("%s: Invalid touch pin %d for current ESP32 target", toString().c_str(), _config.pin);
            return;
        }

        pinMode(_config.pin, INPUT);
        int value = 0;
        bool touched = readTouched(value);
        _state.value = value;
        _state.touched = touched;

        MLOG_INFO("%s: Setup on touch pin %d (threshold=%d, durationMs=%lu)",
                  toString().c_str(),
                  _config.pin,
                  _config.threshold,
                  _config.durationMs);
    }

    void Touch::teardown()
    {
        Device::teardown();

        if (_config.pin >= 0)
        {
            pinMode(_config.pin, INPUT);
        }

        _isSimulated = false;
        _simulatedTouched = false;
        _hasCandidate = false;
        _candidateTouched = false;
        _candidateSince = 0;

        _state.value = 0;
        _state.touched = false;
        _state.isTouchedChanged = false;
    }

    void Touch::loop()
    {
        Device::loop();

        _state.isTouchedChanged = false;

        if (_config.pin < 0)
        {
            return;
        }

        if (!isValidTouchPin(_config.pin))
        {
            MLOG_WARN("%s: Invalid touch pin %d for current ESP32 target", toString().c_str(), _config.pin);
            return;
        }

        int value = 0;
        bool touched = readTouched(value);
        _state.value = value;

        if (_debug)
        {
            MLOG_INFO("%s: touchRead pin=%d value=%d threshold=%d touched=%s",
                      toString().c_str(),
                      _config.pin,
                      value,
                      _config.threshold,
                      touched ? "true" : "false");
        }

        if (touched == _state.touched)
        {
            _hasCandidate = false;
            return;
        }

        if (!_hasCandidate || _candidateTouched != touched)
        {
            _hasCandidate = true;
            _candidateTouched = touched;
            _candidateSince = millis();
            return;
        }

        if (millis() - _candidateSince < _config.durationMs)
        {
            return;
        }

        commitTouchedState(touched, value);
        _hasCandidate = false;
    }

    std::vector<String> Touch::getPins() const
    {
        if (_config.pin < 0)
        {
            return {};
        }

        return {String(_config.pin)};
    }

    void Touch::addStateToJson(JsonDocument &doc)
    {
        doc["value"] = _state.value;
        doc["touched"] = _state.touched;
        doc["isTouchedChanged"] = _state.isTouchedChanged;
    }

    bool Touch::control(const String &action, JsonObject *args)
    {
        (void)args;

        if (action != "touch" && action != "untouch")
        {
            return false;
        }

        _isSimulated = true;
        _simulatedTouched = (action == "touch");

        int value = _simulatedTouched ? (_config.threshold + 1) : _config.threshold;
        if (value < 0)
        {
            value = 0;
        }

        _hasCandidate = false;
        _candidateTouched = false;
        _candidateSince = 0;

        _state.value = value;
        if (_state.touched != _simulatedTouched)
        {
            commitTouchedState(_simulatedTouched, value);
        }

        return true;
    }

    void Touch::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }

        if (config["pin"].is<int>())
        {
            _config.pin = config["pin"].as<int>();
        }

        if (config["threshold"].is<int>())
        {
            _config.threshold = config["threshold"].as<int>();
        }

        if (config["durationMs"].is<unsigned long>())
        {
            _config.durationMs = config["durationMs"].as<unsigned long>();
        }

        if (_config.durationMs == 0)
        {
            _config.durationMs = 50;
        }

        if (_config.threshold < 0)
        {
            _config.threshold = 0;
        }
    }

    void Touch::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        doc["pin"] = _config.pin;
        doc["threshold"] = _config.threshold;
        doc["durationMs"] = _config.durationMs;
    }

    bool Touch::readTouched(int &value) const
    {
        if (_isSimulated)
        {
            value = _simulatedTouched ? (_config.threshold + 1) : _config.threshold;
            if (value < 0)
            {
                value = 0;
            }
            return _simulatedTouched;
        }

        value = touchRead(_config.pin);
        return value > _config.threshold;
    }

    bool Touch::commitTouchedState(bool touched, int value)
    {
        _state.touched = touched;
        _state.value = value;
        _state.isTouchedChanged = true;
        notifyStateChanged();
        return true;
    }

} // namespace devices
