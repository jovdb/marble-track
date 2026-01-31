/**
 * @file Servo.cpp
 * @brief Servo implementation using Device and composition mixins
 */

#include "devices/Servo.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{

    Servo::Servo(const String &id)
        : Device(id, "servo")
    {
        // Create mutex for thread-safe state access
        _stateMutex = xSemaphoreCreateMutex();
    }

    Servo::~Servo()
    {
        if (_mcpwmChannelIndex >= 0)
        {
            McPwmChannels::release(_mcpwmChannelIndex);
        }

        // Clean up mutex
        if (_stateMutex != nullptr)
        {
            vSemaphoreDelete(_stateMutex);
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

        // Start the RTOS task for animation
        if (!startTask("ServoTask", 4096, 1, 1))
        {
            MLOG_ERROR("%s: Failed to start RTOS task", toString().c_str());
        }
    }

    void Servo::teardown()
    {
        Device::teardown();

        stopTask();

        _isAnimating = false;

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

        // Use default duration if not specified (durationMs < 0 means use default)
        uint32_t duration = (durationMs < 0) ? _config.defaultDurationInMs : static_cast<uint32_t>(durationMs);

        MLOG_INFO("%s: setValue(%.3f) -> duty cycle %.1f%% (range: %.1f%%-%.1f%%), duration: %dms",
                  toString().c_str(), value, dutyCycle, _config.minDutyCycle, _config.maxDutyCycle, duration);

        // Use animated or immediate transition based on duration
        if (duration > 0)
        {
            return setDutyCycleAnimated(dutyCycle, duration);
        }
        else
        {
            return setDutyCycle(dutyCycle);
        }
    }

    bool Servo::stop()
    {
        if (!_isAnimating.load())
        {
            MLOG_INFO("%s: Not animating, stop ignored", toString().c_str());
            return false;
        }

        _isAnimating = false;
        notifyStateChanged();
        MLOG_INFO("%s: Animation stopped", toString().c_str());
        return true;
    }

    void Servo::addStateToJson(JsonDocument &doc)
    {
        // Thread-safe state read
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {
            doc["running"] = _state.running;
            doc["value"] = _state.value;
            if (_state.running)
            {
                doc["targetValue"] = _state.targetValue;
                doc["targetDurationMs"] = _state.targetDurationMs;
            }
            xSemaphoreGive(_stateMutex);
        }
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
            MLOG_ERROR("%s: Failed to initialize MCPWM GPIO: %s", toString().c_str(), esp_err_to_name(gpioErr));
            _isSetup = false;
            return false;
        }

        // Configure MCPWM
        mcpwm_config_t pwm_config = {
            .frequency = _config.frequency,
            .cmpr_a = 0.0, // Start with 0% duty cycle
            .cmpr_b = 0.0,
            .duty_mode = MCPWM_DUTY_MODE_0,
            .counter_mode = MCPWM_UP_COUNTER,
        };

        esp_err_t err = mcpwm_init(_mcpwmUnit, _mcpwmTimer, &pwm_config);
        if (err != ESP_OK)
        {
            MLOG_ERROR("%s: MCPWM initialization failed: %s", toString().c_str(), esp_err_to_name(err));
            _isSetup = false;
            return false;
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
            // Update state
            xSemaphoreTake(_stateMutex, portMAX_DELAY);
            _state.value = (dutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle) * 100.0f;
            _state.running = false;
            xSemaphoreGive(_stateMutex);

            notifyStateChanged();
        }
        return true;
    }

    bool Servo::setDutyCycleAnimated(float dutyCycle, uint32_t durationMs)
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

        // If duration is 0, just set immediately
        if (durationMs == 0)
        {
            return setDutyCycle(dutyCycle);
        }

        // Setup animation
        _startDutyCycle = _currentDutyCycle.load();
        _targetDutyCycle = dutyCycle;
        _animationStartTime = millis();
        _animationDuration = durationMs;
        _isAnimating = true;

        // Update state
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.running = true;
        _state.targetValue = (dutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle) * 100.0f;
        _state.targetDurationMs = durationMs;
        xSemaphoreGive(_stateMutex);

        MLOG_INFO("%s: Moving from %.1f%% to %.1f%% over %dms",
                  toString().c_str(), _startDutyCycle.load(), _targetDutyCycle.load(), durationMs);

        // Notify that animation has started
        notifyStateChanged();

        // Wake up the task to start animation
        notifyTask();

        return true;
    }

    void Servo::updateAnimation()
    {
        if (!_isAnimating.load())
            return;

        uint32_t currentTime = millis();
        uint32_t startTime = _animationStartTime.load();
        uint32_t duration = _animationDuration.load();
        uint32_t elapsed = currentTime - startTime;

        // Check if animation is complete
        if (elapsed >= duration)
        {
            _isAnimating = false;
            setDutyCycle(_targetDutyCycle.load(), false);

            // Update state
            xSemaphoreTake(_stateMutex, portMAX_DELAY);
            _state.running = false;
            _state.targetValue = 0.0f;
            _state.targetDurationMs = 0;
            xSemaphoreGive(_stateMutex);

            MLOG_INFO("%s: Target of %.1f%% reached", toString().c_str(), _targetDutyCycle.load());
            notifyStateChanged();
            return;
        }

        // Calculate animation progress (0.0 to 1.0)
        float progress = static_cast<float>(elapsed) / static_cast<float>(duration);

        // Apply easing function
        float easedProgress = easeInOutQuad(progress);

        // Interpolate between start and target duty cycle
        float startDc = _startDutyCycle.load();
        float targetDc = _targetDutyCycle.load();
        float currentDutyCycle = startDc + ((targetDc - startDc) * easedProgress);

        // Apply the interpolated duty cycle directly
        _currentDutyCycle = currentDutyCycle;

        // Set the duty cycle using MCPWM
        esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, currentDutyCycle);
        if (err != ESP_OK)
        {
            MLOG_ERROR("%s: Failed to set animated duty cycle: %s", toString().c_str(), esp_err_to_name(err));
            _isAnimating = false;
            return;
        }

        // Update duty cycle type to percentage
        mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, MCPWM_DUTY_MODE_0);

        // Update state with current value and remaining time
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.value = (currentDutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle) * 100.0f;
        _state.targetDurationMs = duration - elapsed;
        xSemaphoreGive(_stateMutex);
    }

    float Servo::easeInOutQuad(float t)
    {
        // Clamp input to valid range
        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        // Ease-in-out quadratic function
        if (t < 0.5f)
        {
            return 2.0f * t * t;
        }
        else
        {
            return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        }
    }

    /**
     * @brief RTOS task for servo animation
     */
    void Servo::task()
    {
        MLOG_DEBUG("%s: RTOS task started", toString().c_str());

        while (true)
        {
            if (_isAnimating.load())
            {
                updateAnimation();
                // Small delay for smooth animation
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            else
            {
                // Sleep until notified
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            }
        }
    }

} // namespace devices
