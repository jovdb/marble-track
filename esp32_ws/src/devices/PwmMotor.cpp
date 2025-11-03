#include "devices/PwmMotor.h"
#include "Logging.h"
#include <ArduinoJson.h>

PwmMotor::PwmMotor(const String &id, NotifyClients callback)
    : Device(id, "pwmmotor", callback),
      _pin(-1),
      _pwmChannel(0),
      _frequency(50),
      _resolutionBits(10),
      _currentDutyCycle(0.0),
      _isSetup(false),
      _minDutyCycle(5.0),
      _maxDutyCycle(10.0),
      _defaultDurationInMs(1000),
      _isAnimating(false),
      _startDutyCycle(0.0),
      _targetDutyCycle(0.0),
      _animationStartTime(0),
      _animationDuration(0),
      _mcpwmUnit(MCPWM_UNIT_0),
      _mcpwmTimer(MCPWM_TIMER_0),
      _mcpwmSignal(MCPWM0A)
{
}

bool PwmMotor::setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits)
{
    if (pin < 0)
    {
        MLOG_ERROR("PwmMotor [%s]: Invalid pin %d. Pin must be >= 0.", _id.c_str(), pin);
        _isSetup = false;
        _pin = -1;
        return false;
    }

    if (pwmChannel < 0 || pwmChannel > 1)
    {
        MLOG_ERROR("PwmMotor [%s]: Invalid PWM channel %d. Must be 0 or 1.", _id.c_str(), pwmChannel);
        _isSetup = false;
        return false;
    }

    if (frequency == 0)
    {
        MLOG_WARN("PwmMotor [%s]: Frequency cannot be 0. Falling back to 5000 Hz.", _id.c_str());
        frequency = 5000;
    }

    if (resolutionBits < 1 || resolutionBits > 16)
    {
        MLOG_WARN("PwmMotor [%s]: Resolution %d out of range. Clamping between 1 and 16.", _id.c_str(), resolutionBits);
        int clamped = static_cast<int>(resolutionBits);
        if (clamped < 1)
        {
            clamped = 1;
        }
        else if (clamped > 16)
        {
            clamped = 16;
        }
        resolutionBits = static_cast<uint8_t>(clamped);
    }

    _pin = pin;
    _pwmChannel = pwmChannel;
    _frequency = frequency;
    _resolutionBits = resolutionBits;

    // Determine MCPWM unit and timer based on channel
    if (_pwmChannel == 0)
    {
        _mcpwmUnit = MCPWM_UNIT_0;
        _mcpwmTimer = MCPWM_TIMER_0;
        _mcpwmSignal = MCPWM0A;
    }
    else if (_pwmChannel == 1)
    {
        _mcpwmUnit = MCPWM_UNIT_0;
        _mcpwmTimer = MCPWM_TIMER_1;
        _mcpwmSignal = MCPWM1A;
    }

    MLOG_INFO("PwmMotor [%s]: Setup - pin:%d, channel:%d, freq:%d Hz, resolution:%d bits",
              _id.c_str(), _pin, _pwmChannel, _frequency, _resolutionBits);

    bool configured = configureMCPWM();
    if (configured)
    {
        notifyStateChange();
    }

    return configured;
}

bool PwmMotor::configureMCPWM()
{
    if (_pin < 0)
    {
        MLOG_WARN("PwmMotor [%s]: Cannot configure MCPWM without a valid pin.", _id.c_str());
        _isSetup = false;
        return false;
    }

    // Configure GPIO for MCPWM
    esp_err_t gpioErr = mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _pin);
    if (gpioErr != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to initialize MCPWM GPIO: %s", _id.c_str(), esp_err_to_name(gpioErr));
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
        MLOG_ERROR("PwmMotor [%s]: MCPWM initialization failed: %s", _id.c_str(), esp_err_to_name(err));
        _isSetup = false;
        return false;
    }

    _isSetup = true;
    MLOG_INFO("PwmMotor [%s]: MCPWM configured successfully on pin %d", _id.c_str(), _pin);
    return true;
}

bool PwmMotor::setDutyCycle(float dutyCycle, bool notifyChange)
{
    if (!_isSetup)
    {
        MLOG_ERROR("PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return false;
    }

    // Clamp duty cycle to valid range
    if (dutyCycle < 0.0)
        dutyCycle = 0.0;
    if (dutyCycle > 100.0)
        dutyCycle = 100.0;

    _currentDutyCycle = dutyCycle;

    // Set the duty cycle using MCPWM
    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, dutyCycle);
    if (err != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to set duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        return false;
    }

    // Update duty cycle type to percentage
    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    MLOG_INFO("PwmMotor [%s]: Duty cycle set to %.1f%%", _id.c_str(), dutyCycle);

    // Notify state change for WebSocket updates
    if (notifyChange)
    {
        notifyStateChange();
    }
    return true;
}

bool PwmMotor::setDutyCycleAnimated(float dutyCycle, uint32_t durationMs)
{
    if (!_isSetup)
    {
        MLOG_ERROR("PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return false;
    }

    // Clamp duty cycle to valid range
    if (dutyCycle < 0.0)
        dutyCycle = 0.0;
    if (dutyCycle > 100.0)
        dutyCycle = 100.0;

    // If duration is 0, just set immediately
    if (durationMs == 0)
    {
        return setDutyCycle(dutyCycle);
    }

    // Setup animation
    _startDutyCycle = _currentDutyCycle;
    _targetDutyCycle = dutyCycle;
    _animationStartTime = millis();
    _animationDuration = durationMs;
    _isAnimating = true;

    MLOG_INFO("PwmMotor [%s]: Starting animated transition from %.1f%% to %.1f%% over %dms",
              _id.c_str(), _startDutyCycle, _targetDutyCycle, durationMs);

    // Notify that animation has started with target value and duration
    notifyStateChange();
    return true;
}

bool PwmMotor::setValue(float value, int durationMs)
{
    if (!_isSetup)
    {
        MLOG_ERROR("PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
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

    MLOG_INFO("PwmMotor [%s]: setValue(%.3f) -> duty cycle %.1f%% (range: %.1f%%-%.1f%%), duration: %dms",
              _id.c_str(), value, dutyCycle, _minDutyCycle, _maxDutyCycle, duration);

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

float PwmMotor::getValue() const
{
    // Calculate current value as percentage (0-100%) from duty cycle range
    float currentValue = 0.0f;
    if (_maxDutyCycle > _minDutyCycle)
    {
        float normalizedValue = (_currentDutyCycle - _minDutyCycle) / (_maxDutyCycle - _minDutyCycle);
        // Clamp to valid range
        if (normalizedValue < 0.0f) normalizedValue = 0.0f;
        if (normalizedValue > 1.0f) normalizedValue = 1.0f;
        currentValue = normalizedValue * 100.0f;
    }
    return currentValue;
}

void PwmMotor::stop()
{
    setDutyCycle(0.0);
    MLOG_INFO("PwmMotor [%s]: Motor stopped", _id.c_str());
}

void PwmMotor::setup()
{
    Device::setup();

    if (_pin == -1)
    {
        // MLOG_WARN("PwmMotor [%s]: No pin configured. Use setupMotor() to configure.", _id.c_str());
        return;
    }

    // If motor was already configured via setupMotor(), just ensure it's ready
    if (!_isSetup)
    {
        configureMCPWM();
    }
}

void PwmMotor::loop()
{
    Device::loop();

    // Handle animation updates
    if (_isAnimating)
    {
        updateAnimation();
    }
}

bool PwmMotor::control(const String &action, JsonObject *args)
{
    if (action == "stop")
    {
        stop();
        return true;
    }
    else if (action == "setup")
    {
        if (!args)
        {
            MLOG_ERROR("PwmMotor [%s]: 'setup' requires parameters", _id.c_str());
            return false;
        }

        int pin = (*args)["pin"].as<int>();
        int channel = (*args)["channel"].as<int>();
        uint32_t frequency = (*args)["frequency"].as<uint32_t>();
        uint8_t resolutionBits = (*args)["resolutionBits"].as<uint8_t>();

        return setupMotor(pin, channel, frequency, resolutionBits);
    }
    else if (action == "setValue")
    {
        if (!args || !(*args)["value"].is<float>())
        {
            MLOG_ERROR("PwmMotor [%s]: Invalid 'setValue' payload", _id.c_str());
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
        MLOG_WARN("PwmMotor [%s]: Unknown action: %s", _id.c_str(), action.c_str());
        return false;
    }
}

String PwmMotor::getState()
{
    JsonDocument doc;

    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }

    // Add PwmMotor specific state
    doc["running"] = _isAnimating;

    // Calculate current value (0-100%) from duty cycle range
    float currentValue = 0.0f;
    if (_maxDutyCycle > _minDutyCycle)
    {
        float normalizedValue = (_currentDutyCycle - _minDutyCycle) / (_maxDutyCycle - _minDutyCycle);
        // Clamp to valid range
        if (normalizedValue < 0.0f) normalizedValue = 0.0f;
        if (normalizedValue > 1.0f) normalizedValue = 1.0f;
        currentValue = normalizedValue * 100.0f;
    }
    doc["value"] = currentValue;

    if (_isAnimating)
    {
        // Calculate target value (0-100%) from duty cycle range
        float targetValue = 0.0f;
        if (_maxDutyCycle > _minDutyCycle)
        {
            float normalizedValue = (_targetDutyCycle - _minDutyCycle) / (_maxDutyCycle - _minDutyCycle);
            // Clamp to valid range
            if (normalizedValue < 0.0f) normalizedValue = 0.0f;
            if (normalizedValue > 1.0f) normalizedValue = 1.0f;
            targetValue = normalizedValue * 100.0f;
        }
        doc["targetValue"] = targetValue;
        doc["targetDurationMs"] = _animationDuration - (millis() - _animationStartTime);
    }

    String result;
    serializeJson(doc, result);
    return result;
}

String PwmMotor::getConfig() const
{
    JsonDocument config;
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;
    config["pwmChannel"] = _pwmChannel;
    config["frequency"] = _frequency;
    config["resolutionBits"] = _resolutionBits;
    config["minDutyCycle"] = _minDutyCycle;
    config["maxDutyCycle"] = _maxDutyCycle;
    config["defaultDurationInMs"] = _defaultDurationInMs;

    String message;
    serializeJson(config, message);
    return message;
}

void PwmMotor::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("PwmMotor [%s]: Null config provided", _id.c_str());
        return;
    }

    if ((*config)["name"].is<String>())
    {
        _name = (*config)["name"].as<String>();
    }

    int nextPin = _pin;
    if ((*config)["pin"].is<int>())
    {
        nextPin = (*config)["pin"].as<int>();
    }

    int nextChannel = _pwmChannel;
    if ((*config)["pwmChannel"].is<int>())
    {
        nextChannel = (*config)["pwmChannel"].as<int>();
    }
    else if ((*config)["channel"].is<int>())
    {
        nextChannel = (*config)["channel"].as<int>();
    }

    uint32_t nextFrequency = _frequency;
    if ((*config)["frequency"].is<uint32_t>())
    {
        nextFrequency = (*config)["frequency"].as<uint32_t>();
    }
    else if ((*config)["frequency"].is<int>())
    {
        nextFrequency = static_cast<uint32_t>((*config)["frequency"].as<int>());
    }

    uint8_t nextResolution = _resolutionBits;
    if ((*config)["resolutionBits"].is<int>())
    {
        nextResolution = static_cast<uint8_t>((*config)["resolutionBits"].as<int>());
    }

    if ((*config)["minDutyCycle"].is<float>())
    {
        _minDutyCycle = (*config)["minDutyCycle"].as<float>();
    }

    if ((*config)["maxDutyCycle"].is<float>())
    {
        _maxDutyCycle = (*config)["maxDutyCycle"].as<float>();
    }

    if ((*config)["defaultDurationInMs"].is<uint32_t>())
    {
        _defaultDurationInMs = (*config)["defaultDurationInMs"].as<uint32_t>();
    }
    else if ((*config)["defaultDurationInMs"].is<int>())
    {
        _defaultDurationInMs = static_cast<uint32_t>((*config)["defaultDurationInMs"].as<int>());
    }

    _pin = nextPin;
    _pwmChannel = nextChannel;
    _frequency = nextFrequency;
    _resolutionBits = nextResolution;

    if (_pin >= 0)
    {
        setupMotor(_pin, _pwmChannel, _frequency, _resolutionBits);
    }

    notifyStateChange();
}

void PwmMotor::updateAnimation()
{
    if (!_isAnimating)
        return;

    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _animationStartTime;

    // Check if animation is complete
    if (elapsed >= _animationDuration)
    {
        _isAnimating = false;
        bool success = setDutyCycle(_targetDutyCycle, false); // Don't notify yet, we'll do it below
        MLOG_INFO("PwmMotor [%s]: Animation complete, final duty cycle: %.1f%%",
                  _id.c_str(), _targetDutyCycle);
        // Always notify clients when animation completes, regardless of setDutyCycle success
        notifyStateChange();
        return;
    }

    // Calculate animation progress (0.0 to 1.0)
    float progress = (float)elapsed / (float)_animationDuration;

    // Apply easing function
    float easedProgress = easeInOutQuad(progress);

    // Interpolate between start and target duty cycle
    float currentDutyCycle = _startDutyCycle + ((_targetDutyCycle - _startDutyCycle) * easedProgress);

    // Apply the interpolated duty cycle directly (bypass setDutyCycle to avoid recursive calls and notifications)
    _currentDutyCycle = currentDutyCycle;

    // Set the duty cycle using MCPWM
    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, currentDutyCycle);
    if (err != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to set animated duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        _isAnimating = false;
        return;
    }

    // Update duty cycle type to percentage
    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    // Remove periodic notifications during animation to reduce WebSocket traffic
    // Only notify at start (in setDutyCycleAnimated) and end (in this method when complete)
}

float PwmMotor::easeInOutQuad(float t)
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

std::vector<int> PwmMotor::getPins() const
{
    if (_pin < 0)
    {
        return {};
    }

    return {_pin};
}