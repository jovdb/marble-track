/**
 * @file Led.cpp
 * @brief Simple LED implementation using Device, ConfigMixin, and StateMixin
 */

#include "devices/Led.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include <limits>

namespace devices
{
    static bool _isPrevBlinkingOn = false;

    static unsigned long patternDuration(const std::vector<unsigned long> &pattern)
    {
        unsigned long duration = 0;
        for (unsigned long phase : pattern)
            duration += phase;
        return duration;
    }

    static bool patternIsOnAt(const std::vector<unsigned long> &pattern, unsigned long offset, unsigned long &phaseRemaining)
    {
        unsigned long phaseEnd = 0;
        for (size_t index = 0; index < pattern.size(); index++)
        {
            phaseEnd += pattern[index];
            if (offset < phaseEnd)
            {
                phaseRemaining = phaseEnd - offset;
                return index % 2 == 0;
            }
        }

        phaseRemaining = 0;
        return false;
    }

    static bool reached(unsigned long now, unsigned long target)
    {
        return static_cast<long>(now - target) >= 0;
    }

    static bool prefixesMatch(const std::vector<unsigned long> &currentPattern,
                              const std::vector<unsigned long> &newPattern,
                              unsigned long elapsed)
    {
        const unsigned long newDuration = patternDuration(newPattern);
        unsigned long cursor = 0;

        while (cursor < elapsed)
        {
            unsigned long currentPhaseRemaining = 0;
            const bool currentIsOn = patternIsOnAt(currentPattern, cursor, currentPhaseRemaining);

            unsigned long newPhaseRemaining = 0;
            const bool newIsOn = patternIsOnAt(newPattern, cursor % newDuration, newPhaseRemaining);
            const unsigned long segmentDuration = min(currentPhaseRemaining, elapsed - cursor);

            if (currentIsOn != newIsOn || newPhaseRemaining < segmentDuration)
                return false;

            cursor += segmentDuration;
        }

        return true;
    }

    Led::Led(const String &id)
        : Device(id, "led"), _pin(nullptr), _isPrevBlinkingOn(-1)
    {
    }

    Led::~Led()
    {
        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }
    }

    void Led::setup()
    {
        Device::setup();

        // Set the device name
        setName(_config.name);

        // Clean up any existing pin
        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }

        if (_config.pinConfig.pin < 0)
        {
            MLOG_WARN("%s: Setup: Pin not configured", toString().c_str());
            setError("CONFIG_ERROR", "Pin not configured");
            return;
        }

        // Create the pin using the factory
        _pin = PinFactory::createPin(_config.pinConfig);
        if (!_pin)
        {
            MLOG_ERROR("%s: Failed to create pin for expander '%s'", toString().c_str(), _config.pinConfig.expanderId.c_str());
            setError("CONFIG_ERROR", "Failed to create pin for expander '" + _config.pinConfig.expanderId + "'");
            return;
        }

        if (!_pin->setup(_config.pinConfig.pin, pins::PinMode::Output))
        {
            MLOG_ERROR("%s: Failed to setup pin %s", toString().c_str(), _config.pinConfig.toString().c_str());
            setError("CONFIG_ERROR", "Failed to setup pin " + _config.pinConfig.toString());
            // delete _pin;
            //    _pin = nullptr;
            return;
        }
        MLOG_INFO("%s: Setup on %s", toString().c_str(), _pin->toString().c_str());

        clearError();
        // Apply initial state
        // Delay until first loop so all devices are setup?
        if (_config.initialState == "ON")
        {
            set(true);
        }
        else if (_config.initialState == "BLINKING")
        {
            blink(_state.blinkOnTime, _state.blinkOffTime);
        }
        else
        {
            set(false);
        }
    }

    void Led::teardown()
    {
        Device::teardown();

        if (_config.pinConfig.expanderId.isEmpty() && _config.pinConfig.pin >= 0)
        {
            pinMode(_config.pinConfig.pin, INPUT);
        }

        if (_pin)
        {
            delete _pin;
            _pin = nullptr;
        }

        _state.mode = "OFF";
        _state.blinkOnTime = 500;
        _state.blinkOffTime = 500;
        _state.blinkDelay = 0;
        _state.pattern.clear();
        clearPendingPattern();
        _patternStartedAt = 0;
        _isPrevBlinkingOn = -1;
        clearError();
    }

    std::vector<String> Led::getPins() const
    {
        if (_pin)
        {
            String pinStr = _pin->toString();
            if (!pinStr.isEmpty())
                return {pinStr};
            MLOG_DEBUG("Led::getPins: Pin string is empty");
        }
        return {};
    }

    bool Led::set(bool value)
    {
        if (_pin == nullptr || !_pin->isConfigured())
        {
            //           MLOG_WARN("%s: Set: Pin not configured", toString().c_str());
            return false;
        }

        _state.pattern.clear();
        clearPendingPattern();
        _patternStartedAt = 0;

        // Skip if already OK
        if (_state.mode == (value ? "ON" : "OFF"))
        {
            return true;
        }

        // Update state
        _state.mode = value ? "ON" : "OFF";

        _pin->write(value ? HIGH : LOW);
        MLOG_INFO("%s: Set to %s", toString().c_str(), value ? "ON" : "OFF");

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    bool Led::blink(unsigned long onTime, unsigned long offTime, unsigned long delay)
    {
        if (_pin == nullptr || !_pin->isConfigured())
        {
            // MLOG_WARN("%s: Blink: Pin not configured", toString().c_str());
            return false;
        }

        // Skip if already OK
        if (_state.mode == "BLINKING" && _state.blinkOnTime == onTime && _state.blinkOffTime == offTime && _state.blinkDelay == delay)
        {
            return true;
        }

        // Update state
        _state.mode = "BLINKING";
        _state.blinkOnTime = onTime;
        _state.blinkOffTime = offTime;
        _state.blinkDelay = delay;
        _state.pattern.clear();
        clearPendingPattern();

        // Pin set by loop()
        MLOG_INFO("%s: Blinking with delay=%lums, on=%lums, off=%lums (total cycle: %lums)",
                  toString().c_str(), delay, onTime, offTime, delay + onTime + offTime);

        // Notify subscribers
        notifyStateChanged();

        return true;
    }

    void Led::clearPendingPattern()
    {
        _pendingPattern.clear();
        _pendingPatternStartAt = 0;
        _hasPendingPattern = false;
    }

    bool Led::pattern(const std::vector<int> &timings)
    {
        if (_pin == nullptr || !_pin->isConfigured() || timings.size() < 2 || timings.size() > 32 || timings.size() % 2 != 0)
        {
            return false;
        }

        unsigned long totalDuration = 0;
        std::vector<unsigned long> validatedPattern;
        validatedPattern.reserve(timings.size());

        for (int timing : timings)
        {
            if (timing <= 0 || totalDuration > std::numeric_limits<unsigned long>::max() - static_cast<unsigned long>(timing))
            {
                return false;
            }

            validatedPattern.push_back(static_cast<unsigned long>(timing));
            totalDuration += static_cast<unsigned long>(timing);
        }

        if (_state.mode == "PATTERN" && _state.pattern == validatedPattern)
        {
            return true;
        }

        const unsigned long now = millis();

        if (_state.mode != "PATTERN" || _state.pattern.empty())
        {
            clearPendingPattern();
            _state.mode = "PATTERN";
            _state.pattern = validatedPattern;
            _patternStartedAt = now;
            _isPrevBlinkingOn = -1;
            MLOG_INFO("%s: Pattern configured with %u phases, total duration=%lums", toString().c_str(), static_cast<unsigned>(timings.size()), totalDuration);
            notifyStateChanged();
            return true;
        }

        const unsigned long currentDuration = patternDuration(_state.pattern);
        const unsigned long elapsed = (now - _patternStartedAt) % currentDuration;

        if (prefixesMatch(_state.pattern, validatedPattern, elapsed))
        {
            _state.pattern = validatedPattern;
            _patternStartedAt = now - elapsed;
            clearPendingPattern();
            _isPrevBlinkingOn = -1;
            notifyStateChanged();
            return true;
        }

        _state.pattern = validatedPattern;
        _patternStartedAt = now;
        clearPendingPattern();
        _isPrevBlinkingOn = -1;
        notifyStateChanged();
        return true;
    }

    void Led::loop()
    {
        Device::loop();

        if (_pin == nullptr || !_pin->isConfigured() || (_state.mode != "BLINKING" && _state.mode != "PATTERN"))
        {
            _isPrevBlinkingOn = -1;
            return;
        }

        if (_state.mode == "PATTERN")
        {
            const unsigned long now = millis();

            if (_hasPendingPattern && reached(now, _pendingPatternStartAt))
            {
                _state.pattern = _pendingPattern;
                _patternStartedAt = _pendingPatternStartAt;
                clearPendingPattern();
                _isPrevBlinkingOn = -1;
                notifyStateChanged();
            }

            unsigned long totalDuration = patternDuration(_state.pattern);

            if (totalDuration == 0)
                return;

            const unsigned long value = (now - _patternStartedAt) % totalDuration;
            unsigned long phaseRemaining = 0;
            const bool shouldBeOn = patternIsOnAt(_state.pattern, value, phaseRemaining);

            if ((shouldBeOn && _isPrevBlinkingOn != 1) || (!shouldBeOn && _isPrevBlinkingOn != 0))
            {
                _isPrevBlinkingOn = shouldBeOn ? 1 : 0;
                _pin->write(shouldBeOn ? HIGH : LOW);
            }
            return;
        }

        // Calculate total blink cycle time (including delay)
        unsigned long cycle = _state.blinkDelay + _state.blinkOnTime + _state.blinkOffTime;

        // Use modulo to find value in cycle (0 to cycle-1)
        unsigned long value = millis() % cycle;

        // Determine LED state based on value in cycle:
        // 0 to delay-1: OFF (delay period)
        // delay to delay+onTime-1: ON (on period)
        // delay+onTime to cycle-1: OFF (off period)
        bool shouldBeOn = (value >= _state.blinkDelay && value < _state.blinkDelay + _state.blinkOnTime);

        // Only update GPIO if state changed (check against current mode state)
        if ((shouldBeOn && _isPrevBlinkingOn != 1) || (!shouldBeOn && _isPrevBlinkingOn != 0))
        {
            _isPrevBlinkingOn = shouldBeOn ? 1 : 0;
            _pin->write(shouldBeOn ? HIGH : LOW);
        }
    }

    void Led::addDeviceStateToJson(JsonDocument &doc)
    {
        doc["mode"] = _state.mode;
        doc["blinkOnTime"] = _state.blinkOnTime;
        doc["blinkOffTime"] = _state.blinkOffTime;
        doc["blinkDelay"] = _state.blinkDelay;
        if (_state.mode == "PATTERN")
        {
            JsonArray patternArray = doc["pattern"].to<JsonArray>();
            for (unsigned long duration : _state.pattern)
                patternArray.add(duration);
        }
    }

    bool Led::control(const String &action, JsonObject *args)
    {
        if (action == "set")
        {
            if (!args || !(*args)["value"].is<bool>())
            {
                MLOG_WARN("%s: 'set' action requires 'value' argument", toString().c_str());
                return false;
            }
            bool value = (*args)["value"].as<bool>();
            return set(value);
        }
        else if (action == "blink")
        {
            unsigned long onTime = 500;
            unsigned long offTime = 500;
            unsigned long delay = 0;

            if (args)
            {
                if ((*args)["onTime"].is<unsigned long>())
                    onTime = (*args)["onTime"].as<unsigned long>();
                if ((*args)["offTime"].is<unsigned long>())
                    offTime = (*args)["offTime"].as<unsigned long>();
                if ((*args)["delay"].is<unsigned long>())
                    delay = (*args)["delay"].as<unsigned long>();
            }

            return blink(onTime, offTime, delay);
        }
        else if (action == "pattern")
        {
            if (!args || !(*args)["pattern"].is<JsonArrayConst>())
                return false;

            JsonArrayConst patternArray = (*args)["pattern"].as<JsonArrayConst>();
            std::vector<int> timings;
            for (JsonVariantConst value : patternArray)
            {
                if (!value.is<int>())
                    return false;
                timings.push_back(value.as<int>());
            }
            return pattern(timings);
        }
        else
        {
            MLOG_WARN("%s: Unknown control action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Led::jsonToConfig(const JsonDocument &config)
    {

        _config.pinConfig = PinFactory::jsonToConfig(config["pin"]);

        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
        if (config["initialState"].is<String>())
        {
            _config.initialState = config["initialState"].as<String>();
        }
    }

    void Led::configToJson(JsonDocument &doc)
    {
        JsonDocument pinDoc;
        PinFactory::configToJson(_config.pinConfig, pinDoc);

        doc["pin"] = pinDoc.as<JsonVariant>();
        doc["name"] = _config.name;
        doc["initialState"] = _config.initialState;
    }

} // namespace devices
