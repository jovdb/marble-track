#include "devices/Servo.h"
#include "Logging.h"
#include <ArduinoJson.h>

Servo::Servo(const String &id, NotifyClients callback)
    : ControllableTaskDevice(id, "servo", callback),
      _name("Servo"),
      _pin(-1),
      _mcpwmChannelIndex(-1),
      _frequency(50),
      _resolutionBits(10),
      _minDutyCycle(2.5),
      _maxDutyCycle(12.5),
      _defaultDurationInMs(500),
      _isSetup(false),
      _currentDutyCycle(0.0f),
      _isAnimating(false),
      _startDutyCycle(0.0f),
      _targetDutyCycle(0.0f),
      _animationStartTime(0),
      _animationDuration(0),
      _mcpwmUnit(MCPWM_UNIT_0),
      _mcpwmTimer(MCPWM_TIMER_0),
      _mcpwmSignal(MCPWM0A),
      _mcpwmOperator(MCPWM_OPR_A)
{
}

Servo::~Servo()
{
    if (_mcpwmChannelIndex >= 0)
    {
        McPwmChannels::release(_mcpwmChannelIndex);
        _mcpwmChannelIndex = -1;
    }
}

void Servo::getConfigFromJson(const JsonDocument &config)
{
    if (config["name"].is<String>())
    {
        _name = config["name"].as<String>();
    }

    if (config["pin"].is<int>())
    {
        _pin = config["pin"].as<int>();
    }

    if (config["mcpwmChannel"].is<int>())
    {
        _mcpwmChannelIndex = config["mcpwmChannel"].as<int>();
    }

    if (config["frequency"].is<uint32_t>())
    {
        _frequency = config["frequency"].as<uint32_t>();
    }
    else if (config["frequency"].is<int>())
    {
        _frequency = static_cast<uint32_t>(config["frequency"].as<int>());
    }

    if (config["resolutionBits"].is<int>())
    {
        _resolutionBits = static_cast<uint8_t>(config["resolutionBits"].as<int>());
    }

    if (config["minDutyCycle"].is<float>())
    {
        _minDutyCycle = config["minDutyCycle"].as<float>();
    }

    if (config["maxDutyCycle"].is<float>())
    {
        _maxDutyCycle = config["maxDutyCycle"].as<float>();
    }

    if (config["defaultDurationInMs"].is<uint32_t>())
    {
        _defaultDurationInMs = config["defaultDurationInMs"].as<uint32_t>();
    }
    else if (config["defaultDurationInMs"].is<int>())
    {
        _defaultDurationInMs = static_cast<uint32_t>(config["defaultDurationInMs"].as<int>());
    }

    if (_pin >= 0)
    {
        setupServo();
    }
    else
    {
        MLOG_WARN("%s: No valid pin configured", toString().c_str());
    }
}

void Servo::addConfigToJson(JsonDocument &doc) const
{
    doc["name"] = _name;
    doc["pin"] = _pin;
    doc["mcpwmChannel"] = _mcpwmChannelIndex;
    doc["frequency"] = _frequency;
    doc["resolutionBits"] = _resolutionBits;
    doc["minDutyCycle"] = _minDutyCycle;
    doc["maxDutyCycle"] = _maxDutyCycle;
    doc["defaultDurationInMs"] = _defaultDurationInMs;
}

void Servo::addStateToJson(JsonDocument &doc)
{
    doc["running"] = _isAnimating.load();

    // Calculate current value (0-100%) from duty cycle range
    float currentValue = getValue();
    doc["value"] = currentValue;

    if (_isAnimating.load())
    {
        // Calculate target value (0-100%) from duty cycle range
        float targetDutyCycle = _targetDutyCycle.load();
        float targetValue = 0.0f;
        if (_maxDutyCycle > _minDutyCycle)
        {
            float normalizedValue = (targetDutyCycle - _minDutyCycle) / (_maxDutyCycle - _minDutyCycle);
            if (normalizedValue < 0.0f)
                normalizedValue = 0.0f;
            if (normalizedValue > 1.0f)
                normalizedValue = 1.0f;
            targetValue = normalizedValue * 100.0f;
        }
        doc["targetValue"] = targetValue;

        uint32_t startTime = _animationStartTime.load();
        uint32_t duration = _animationDuration.load();
        uint32_t elapsed = millis() - startTime;
        uint32_t remaining = (elapsed < duration) ? (duration - elapsed) : 0;
        doc["targetDurationMs"] = remaining;
    }
}

bool Servo::control(const String &action, JsonObject *args)
{
    if (action == "stop")
    {
        stop();
        return true;
    }
    else if (action == "setValue")
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
    else
    {
        MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
        return false;
    }
}

std::vector<int> Servo::getPins() const
{
    if (_pin < 0)
    {
        return {};
    }
    return {_pin};
}

bool Servo::setupServo()
{
    if (_pin < 0)
    {
        MLOG_WARN("%s: Invalid pin. Pin must be >= 0.", toString().c_str());
        _isSetup = false;
        return false;
    }

    if (_frequency == 0)
    {
        MLOG_WARN("%s: Frequency cannot be 0. Falling back to 50 Hz.", toString().c_str());
        _frequency = 50;
    }

    if (_resolutionBits < 1 || _resolutionBits > 16)
    {
        MLOG_WARN("%s: Resolution %d out of range. Clamping between 1 and 16.", toString().c_str(), _resolutionBits);
        if (_resolutionBits < 1)
            _resolutionBits = 1;
        if (_resolutionBits > 16)
            _resolutionBits = 16;
    }

    // Release old channel if acquired
    if (_mcpwmChannelIndex >= 0)
    {
        McPwmChannels::release(_mcpwmChannelIndex);
        _mcpwmChannelIndex = -1;
    }

    // Acquire new channel
    int channelToUse = _mcpwmChannelIndex;
    if (channelToUse == -1)
    {
        channelToUse = McPwmChannels::acquireFree();
        if (channelToUse == -1)
        {
            MLOG_ERROR("%s: No free MCPWM channels available.", toString().c_str());
            _isSetup = false;
            return false;
        }
    }
    else
    {
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
        notifyStateChange();
    }

    return configured;
}

bool Servo::configureMCPWM()
{
    if (_pin < 0)
    {
        MLOG_WARN("%s: Cannot configure MCPWM without a valid pin.", toString().c_str());
        _isSetup = false;
        return false;
    }

    esp_err_t gpioErr = mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _pin);
    if (gpioErr != ESP_OK)
    {
        MLOG_ERROR("%s: Failed to initialize MCPWM GPIO: %s", toString().c_str(), esp_err_to_name(gpioErr));
        _isSetup = false;
        return false;
    }

    // Configure MCPWM
    mcpwm_config_t pwm_config = {
        .frequency = _frequency,
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

    _isSetup = true;
    MLOG_INFO("%s: MCPWM configured on pin %d, channel %d", toString().c_str(), _pin, _mcpwmChannelIndex);
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
        notifyStateChange();
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

    MLOG_INFO("%s: Starting animated transition from %.1f%% to %.1f%% over %dms",
              toString().c_str(), _startDutyCycle.load(), _targetDutyCycle.load(), durationMs);

    // Notify that animation has started with target value and duration
    notifyStateChange();

    // Wake up the task to start animation
    if (_taskHandle)
    {
        xTaskNotifyGive(_taskHandle);
    }

    return true;
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
    float dutyCycle = _minDutyCycle + (value * (_maxDutyCycle - _minDutyCycle));

    // Use default duration if not specified
    uint32_t duration = (durationMs < 0) ? _defaultDurationInMs : static_cast<uint32_t>(durationMs);

    MLOG_INFO("%s: setValue(%.3f) -> duty cycle %.1f%% (range: %.1f%%-%.1f%%), duration: %dms",
              toString().c_str(), value, dutyCycle, _minDutyCycle, _maxDutyCycle, duration);

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

float Servo::getValue() const
{
    // Calculate current value as percentage (0-100%) from duty cycle range
    float currentValue = 0.0f;
    if (_maxDutyCycle > _minDutyCycle)
    {
        float normalizedValue = (_currentDutyCycle.load() - _minDutyCycle) / (_maxDutyCycle - _minDutyCycle);
        if (normalizedValue < 0.0f)
            normalizedValue = 0.0f;
        if (normalizedValue > 1.0f)
            normalizedValue = 1.0f;
        currentValue = normalizedValue * 100.0f;
    }
    return currentValue;
}

void Servo::stop()
{
    _isAnimating = false;
    notifyStateChange();
    MLOG_INFO("%s: Animation stopped", toString().c_str());
}

void Servo::task()
{
    if (_pin < 0)
    {
        MLOG_ERROR("%s: No valid pin configured, task waiting", toString().c_str());
    }

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
        MLOG_INFO("%s: Animation complete, final duty cycle: %.1f%%",
                  toString().c_str(), _targetDutyCycle.load());
        notifyStateChange();
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
