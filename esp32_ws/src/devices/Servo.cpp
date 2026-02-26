/**
 * @file Servo.cpp
 * @brief Servo implementation using Device and composition mixins
 */

#include "devices/Servo.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    namespace
    {
        constexpr int MCPWM_UNIT_COUNT = 2;
        constexpr int MCPWM_TIMER_COUNT = 3;

        bool s_timerInitialized[MCPWM_UNIT_COUNT][MCPWM_TIMER_COUNT] = {{false}};
        uint32_t s_timerFrequency[MCPWM_UNIT_COUNT][MCPWM_TIMER_COUNT] = {{0}};

        bool resolveMcpwmIndexes(mcpwm_unit_t unit, mcpwm_timer_t timer, int &unitIndex, int &timerIndex)
        {
            unitIndex = static_cast<int>(unit);
            timerIndex = static_cast<int>(timer);

            if (unitIndex < 0 || unitIndex >= MCPWM_UNIT_COUNT)
            {
                return false;
            }

            if (timerIndex < 0 || timerIndex >= MCPWM_TIMER_COUNT)
            {
                return false;
            }

            return true;
        }
    }

    Servo::Servo(const String &id)
        : Device(id, "servo")
    {
    }

    Servo::~Servo()
    {
        if (_mcpwmChannelIndex >= 0)
        {
            McPwmChannels::release(_mcpwmChannelIndex);
        }
    }

    void Servo::setup()
    {
        Device::setup();

        // Determine if auto-assignment was requested
        _wasAutoAssigned = (_config.mcpwmChannel == -1);

        if (_config.pin < 0)
        {
            MLOG_WARN("%s: Pin not configured", toString().c_str());
            return;
        }

        // Set the device name
        setName(_config.name);

        // Setup MCPWM for servo control
        if (!setupServo())
        {
            MLOG_ERROR("%s: Failed to setup servo", toString().c_str());
            return;
        }

        MLOG_INFO("%s: Setup on pin %d, MCPWM channel %d", toString().c_str(), _config.pin, _mcpwmChannelIndex);
    }

    void Servo::teardown()
    {
        Device::teardown();

        _isAnimating = false;
        _animationStartDutyCycle = 0.0f;
        _animationTargetDutyCycle = 0.0f;
        _animationStartTimeMs = 0;
        _animationDurationMs = 0;

        if (_mcpwmChannelIndex >= 0)
        {
            McPwmChannels::release(_mcpwmChannelIndex);
            _mcpwmChannelIndex = -1;
        }

        if (_config.pin >= 0)
        {
            pinMode(_config.pin, INPUT);
        }

        _isSetup = false;
    }

    void Servo::loop()
    {
        Device::loop();

        if (!_isSetup || !_isAnimating)
        {
            return;
        }

        const uint32_t now = millis();
        const uint32_t elapsed = now - _animationStartTimeMs;

        if (elapsed >= _animationDurationMs)
        {
            setDutyCycle(_animationTargetDutyCycle);
            _isAnimating = false;
            return;
        }

        const float progress = static_cast<float>(elapsed) / static_cast<float>(_animationDurationMs);
        const float nextDutyCycle = _animationStartDutyCycle + ((_animationTargetDutyCycle - _animationStartDutyCycle) * progress);
        setDutyCycle(nextDutyCycle, false);
    }

    std::vector<String> Servo::getPins() const
    {
        if (_config.pin == -1)
        {
            return {};
        }
        return {String(_config.pin)};
    }

    bool Servo::setValue(float value, int durationMs)
    {
        if (!_isSetup)
        {
            MLOG_WARN("%s: Not setup. Configure pin first.", toString().c_str());
            return false;
        }

        // Clamp value between 0.0 and 1.0
        if (value < 0.0f)
            value = 0.0f;
        if (value > 1.0f)
            value = 1.0f;

        // Map normalized value (0.0-1.0) to duty cycle range (min-max)
        float dutyCycle = _config.minDutyCycle + (value * (_config.maxDutyCycle - _config.minDutyCycle));

        const uint32_t requestedDurationMs = durationMs >= 0 ? static_cast<uint32_t>(durationMs) : _config.defaultDurationInMs;

        if (requestedDurationMs == 0)
        {
            _isAnimating = false;
            MLOG_INFO("%s: setValue(%.3f) immediate -> duty cycle %.1f%%", toString().c_str(), value, dutyCycle);
            return setDutyCycle(dutyCycle);
        }

        _animationStartDutyCycle = _currentDutyCycle;
        _animationTargetDutyCycle = dutyCycle;
        _animationStartTimeMs = millis();
        _animationDurationMs = requestedDurationMs;
        _isAnimating = true;

        MLOG_INFO("%s: setValue(%.3f) over %lu ms -> duty cycle %.1f%% (range: %.1f%%-%.1f%%)",
                  toString().c_str(), value, static_cast<unsigned long>(_animationDurationMs), dutyCycle, _config.minDutyCycle, _config.maxDutyCycle);

        notifyStateChanged();
        return true;
    }

    bool Servo::stop()
    {
        if (!_isAnimating)
        {
            MLOG_INFO("%s: Stop ignored, no animation", toString().c_str());
            return false;
        }

        _isAnimating = false;
        notifyStateChanged();
        MLOG_INFO("%s: Animation stopped", toString().c_str());
        return true;
    }

    void Servo::addStateToJson(JsonDocument &doc)
    {
        const float currentValue = ((_currentDutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle)) * 100.0f;
        const float targetValue = ((_animationTargetDutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle)) * 100.0f;

        doc["running"] = _isAnimating;
        doc["value"] = currentValue;
        doc["targetValue"] = _isAnimating ? targetValue : currentValue;
        doc["targetDurationMs"] = _isAnimating ? _animationDurationMs : 0;
    }

    bool Servo::control(const String &action, JsonObject *args)
    {
        if (action == "setValue")
        {
            if (!args || !(*args)["value"].is<float>())
            {
                MLOG_WARN("%s: Invalid 'setValue' payload", toString().c_str());
                return false;
            }
            float value = (*args)["value"].as<float>();

            // Check for optional duration parameter
            int durationMs = -1; // Use default
            if ((*args)["durationMs"].is<int>())
            {
                durationMs = (*args)["durationMs"].as<int>();
            }
            else if ((*args)["durationMs"].is<uint32_t>())
            {
                durationMs = static_cast<int>((*args)["durationMs"].as<uint32_t>());
            }

            return setValue(value, durationMs);
        }
        else if (action == "stop")
        {
            return stop();
        }
        else
        {
            MLOG_WARN("%s: Unknown control action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Servo::jsonToConfig(const JsonDocument &config)
    {
        if (config["pin"].is<int>())
        {
            _config.pin = config["pin"].as<int>();
        }
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
        if (config["mcpwmChannel"].is<int>())
        {
            _config.mcpwmChannel = config["mcpwmChannel"].as<int>();
        }
        if (config["frequency"].is<uint32_t>())
        {
            _config.frequency = config["frequency"].as<uint32_t>();
        }
        else if (config["frequency"].is<int>())
        {
            _config.frequency = static_cast<uint32_t>(config["frequency"].as<int>());
        }
        if (config["resolutionBits"].is<int>())
        {
            _config.resolutionBits = static_cast<uint8_t>(config["resolutionBits"].as<int>());
        }
        if (config["minDutyCycle"].is<float>())
        {
            _config.minDutyCycle = config["minDutyCycle"].as<float>();
        }
        if (config["maxDutyCycle"].is<float>())
        {
            _config.maxDutyCycle = config["maxDutyCycle"].as<float>();
        }
        if (config["defaultDurationInMs"].is<uint32_t>())
        {
            _config.defaultDurationInMs = config["defaultDurationInMs"].as<uint32_t>();
        }
        else if (config["defaultDurationInMs"].is<int>())
        {
            _config.defaultDurationInMs = static_cast<uint32_t>(config["defaultDurationInMs"].as<int>());
        }
    }

    void Servo::configToJson(JsonDocument &doc)
    {
        doc["pin"] = _config.pin;
        doc["name"] = _config.name;
        doc["mcpwmChannel"] = _config.mcpwmChannel;
        doc["frequency"] = _config.frequency;
        doc["resolutionBits"] = _config.resolutionBits;
        doc["minDutyCycle"] = _config.minDutyCycle;
        doc["maxDutyCycle"] = _config.maxDutyCycle;
        doc["defaultDurationInMs"] = _config.defaultDurationInMs;
    }

    bool Servo::setupServo()
    {
        if (_config.pin < 0)
        {
            MLOG_WARN("%s: Invalid pin. Pin must be >= 0.", toString().c_str());
            _isSetup = false;
            return false;
        }

        if (_config.frequency == 0)
        {
            MLOG_WARN("%s: Frequency cannot be 0. Falling back to 50 Hz.", toString().c_str());
            _config.frequency = 50;
        }

        if (_config.resolutionBits < 1 || _config.resolutionBits > 16)
        {
            MLOG_WARN("%s: Resolution %d out of range. Clamping between 1 and 16.", toString().c_str(), _config.resolutionBits);
            if (_config.resolutionBits < 1)
                _config.resolutionBits = 1;
            if (_config.resolutionBits > 16)
                _config.resolutionBits = 16;
        }

        // Release old channel if acquired
        if (_mcpwmChannelIndex >= 0)
        {
            McPwmChannels::release(_mcpwmChannelIndex);
            _mcpwmChannelIndex = -1;
        }

        // Acquire new channel
        int channelToUse;
        if (_wasAutoAssigned)
        {
            channelToUse = McPwmChannels::acquireFree();
            if (channelToUse == -1)
            {
                MLOG_ERROR("%s: No free MCPWM channels available.", toString().c_str());
                _isSetup = false;
                return false;
            }
            // Save the assigned channel in config
            _config.mcpwmChannel = channelToUse;
        }
        else
        {
            channelToUse = _config.mcpwmChannel;
            if (!McPwmChannels::acquireSpecific(channelToUse))
            {
                MLOG_ERROR("%s: MCPWM channel %d already in use or invalid.", toString().c_str(), channelToUse);
                _isSetup = false;
                return false;
            }
        }

        _mcpwmChannelIndex = channelToUse;

        // Determine MCPWM unit, timer, signal, and operator based on channel
        _mcpwmUnit = MCPWM_UNIT_0;
        int timerIndex = _mcpwmChannelIndex / 2; // 0-2 for timers 0-2
        _mcpwmTimer = static_cast<mcpwm_timer_t>(timerIndex);
        _mcpwmSignal = McPwmChannels::getSignal(_mcpwmChannelIndex);
        _mcpwmOperator = (_mcpwmChannelIndex % 2 == 0) ? MCPWM_OPR_A : MCPWM_OPR_B;

        MLOG_DEBUG("%s: MCPWM mapping channel=%d timer=%d signal=%d operator=%s pin=%d freq=%lu",
               toString().c_str(), _mcpwmChannelIndex, static_cast<int>(_mcpwmTimer), static_cast<int>(_mcpwmSignal),
               _mcpwmOperator == MCPWM_OPR_A ? "A" : "B", _config.pin, static_cast<unsigned long>(_config.frequency));

        bool configured = configureMCPWM();
        if (configured)
        {
            _isSetup = true;
            notifyStateChanged();
        }

        return configured;
    }

    bool Servo::configureMCPWM()
    {
        if (_config.pin < 0)
        {
            MLOG_WARN("%s: Cannot configure MCPWM without a valid pin.", toString().c_str());
            _isSetup = false;
            return false;
        }

        esp_err_t gpioErr = mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _config.pin);
        if (gpioErr != ESP_OK)
        {
            MLOG_ERROR("%s: Failed to initialize MCPWM pin %s", toString().c_str(), esp_err_to_name(gpioErr));
            _isSetup = false;
            return false;
        }

        MLOG_DEBUG("%s: MCPWM GPIO route unit=%d signal=%d -> pin=%d",
                   toString().c_str(), static_cast<int>(_mcpwmUnit), static_cast<int>(_mcpwmSignal), _config.pin);

        // Configure MCPWM
        mcpwm_config_t pwm_config = {
            .frequency = _config.frequency,
            .cmpr_a = 0.0, // Start with 0% duty cycle
            .cmpr_b = 0.0,
            .duty_mode = MCPWM_DUTY_MODE_0,
            .counter_mode = MCPWM_UP_COUNTER,
        };

        int unitIndex = -1;
        int timerIndex = -1;
        if (!resolveMcpwmIndexes(_mcpwmUnit, _mcpwmTimer, unitIndex, timerIndex))
        {
            MLOG_ERROR("%s: Invalid MCPWM mapping unit=%d timer=%d", toString().c_str(), static_cast<int>(_mcpwmUnit), static_cast<int>(_mcpwmTimer));
            _isSetup = false;
            return false;
        }

        if (!s_timerInitialized[unitIndex][timerIndex])
        {
            esp_err_t err = mcpwm_init(_mcpwmUnit, _mcpwmTimer, &pwm_config);
            if (err != ESP_OK)
            {
                MLOG_ERROR("%s: MCPWM initialization failed: %s", toString().c_str(), esp_err_to_name(err));
                _isSetup = false;
                return false;
            }

            s_timerInitialized[unitIndex][timerIndex] = true;
            s_timerFrequency[unitIndex][timerIndex] = _config.frequency;

            MLOG_DEBUG("%s: Initialized MCPWM timer unit=%d timer=%d frequency=%lu",
                       toString().c_str(), unitIndex, timerIndex, static_cast<unsigned long>(_config.frequency));
        }
        else
        {
            const uint32_t configuredFrequency = s_timerFrequency[unitIndex][timerIndex];
            if (configuredFrequency != _config.frequency)
            {
                MLOG_ERROR("%s: MCPWM timer unit=%d timer=%d already initialized at %lu Hz, requested %lu Hz",
                           toString().c_str(), unitIndex, timerIndex,
                           static_cast<unsigned long>(configuredFrequency),
                           static_cast<unsigned long>(_config.frequency));
                _isSetup = false;
                return false;
            }

            MLOG_DEBUG("%s: Reusing MCPWM timer unit=%d timer=%d at %lu Hz",
                       toString().c_str(), unitIndex, timerIndex, static_cast<unsigned long>(configuredFrequency));
        }

        return true;
    }

    bool Servo::setDutyCycle(float dutyCycle, bool notifyChange)
    {
        if (!_isSetup)
        {
            MLOG_WARN("%s: Not setup. Configure pin first.", toString().c_str());
            return false;
        }

        // Clamp duty cycle to valid range
        if (dutyCycle < 0.0f)
            dutyCycle = 0.0f;
        if (dutyCycle > 100.0f)
            dutyCycle = 100.0f;

        _currentDutyCycle = dutyCycle;

        // Set the duty cycle using MCPWM
        esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, dutyCycle);
        if (err != ESP_OK)
        {
            MLOG_ERROR("%s: Failed to set duty cycle: %s", toString().c_str(), esp_err_to_name(err));
            return false;
        }

        // Update duty cycle type to percentage
        mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, MCPWM_DUTY_MODE_0);

        if (notifyChange)
        {
            notifyStateChanged();
        }
        return true;
    }

} // namespace devices
