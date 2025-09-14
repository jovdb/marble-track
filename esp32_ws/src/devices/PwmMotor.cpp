#include "devices/PwmMotor.h"
#include "esp_log.h"

static const char *TAG = "PwmMotor";

PwmMotor::PwmMotor(const String &id, const String &name)
    : Device(id, "pwmmotor"),
      _pin(-1),
      _pwmChannel(0),
      _frequency(5000),
      _resolutionBits(12),
      _currentDutyCycle(0.0),
      _isSetup(false),
      _isAnimating(false),
      _startDutyCycle(0.0),
      _targetDutyCycle(0.0),
      _animationStartTime(0),
      _animationDuration(0),
      _mcpwmUnit(MCPWM_UNIT_0),
      _mcpwmTimer(MCPWM_TIMER_0),
      _mcpwmSignal(MCPWM0A)
{
    _name = name;
}

bool PwmMotor::setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits)
{
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
    else
    {
        ESP_LOGE(TAG, "PwmMotor [%s]: Invalid PWM channel %d. Must be 0 or 1.", _id.c_str(), _pwmChannel);
        return false;
    }

    ESP_LOGI(TAG, "PwmMotor [%s]: Setup - pin:%d, channel:%d, freq:%d Hz, resolution:%d bits",
             _id.c_str(), _pin, _pwmChannel, _frequency, _resolutionBits);

    return configureMCPWM();
}

bool PwmMotor::configureMCPWM()
{
    // Configure GPIO for MCPWM
    ESP_ERROR_CHECK(mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _pin));

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
        ESP_LOGE(TAG, "PwmMotor [%s]: MCPWM initialization failed: %s", _id.c_str(), esp_err_to_name(err));
        return false;
    }

    _isSetup = true;
    ESP_LOGI(TAG, "PwmMotor [%s]: MCPWM configured successfully on pin %d", _id.c_str(), _pin);
    return true;
}

void PwmMotor::setDutyCycle(float dutyCycle, bool notifyChange)
{
    if (!_isSetup)
    {
        ESP_LOGE(TAG, "PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return;
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
        ESP_LOGE(TAG, "PwmMotor [%s]: Failed to set duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        return;
    }

    // Update duty cycle type to percentage
    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    ESP_LOGD(TAG, "PwmMotor [%s]: Duty cycle set to %.1f%%", _id.c_str(), dutyCycle);

    // Notify state change for WebSocket updates
    if (notifyChange)
    {
        notifyStateChange();
    }
}

void PwmMotor::setDutyCycleAnimated(float dutyCycle, uint32_t durationMs)
{
    if (!_isSetup)
    {
        ESP_LOGE(TAG, "PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return;
    }

    // Clamp duty cycle to valid range
    if (dutyCycle < 0.0)
        dutyCycle = 0.0;
    if (dutyCycle > 100.0)
        dutyCycle = 100.0;

    // If duration is 0, just set immediately
    if (durationMs == 0)
    {
        setDutyCycle(dutyCycle);
        return;
    }

    // Setup animation
    _startDutyCycle = _currentDutyCycle;
    _targetDutyCycle = dutyCycle;
    _animationStartTime = millis();
    _animationDuration = durationMs;
    _isAnimating = true;

    ESP_LOGI(TAG, "PwmMotor [%s]: Starting animated transition from %.1f%% to %.1f%% over %dms",
             _id.c_str(), _startDutyCycle, _targetDutyCycle, durationMs);

    // Notify that animation has started with target value and duration
    notifyStateChange();
}

void PwmMotor::stop()
{
    setDutyCycle(0.0);
    ESP_LOGI(TAG, "PwmMotor [%s]: Motor stopped", _id.c_str());
}

void PwmMotor::setup()
{
    Device::setup();

    if (_pin == -1)
    {
        ESP_LOGW(TAG, "PwmMotor [%s]: No pin configured. Use setupMotor() to configure.", _id.c_str());
        return;
    }

    // If motor was already configured via setupMotor(), just ensure it's ready
    if (!_isSetup)
    {
        configureMCPWM();
    }

    ESP_LOGI(TAG, "PwmMotor [%s]: Device setup complete", _id.c_str());
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
    if (action == "setDutyCycle")
    {
        if (!args || !(*args)["value"].is<float>())
        {
            ESP_LOGE(TAG, "PwmMotor [%s]: Invalid 'setDutyCycle' payload", _id.c_str());
            return false;
        }
        float dutyCycle = (*args)["value"].as<float>();

        // Check for optional duration parameter
        if ((*args)["durationMs"].is<uint32_t>())
        {
            uint32_t durationMs = (*args)["durationMs"].as<uint32_t>();
            setDutyCycleAnimated(dutyCycle, durationMs);
        }
        else
        {
            setDutyCycle(dutyCycle);
        }
        return true;
    }
    else if (action == "stop")
    {
        stop();
        return true;
    }
    else if (action == "setup")
    {
        if (!args)
        {
            ESP_LOGE(TAG, "PwmMotor [%s]: 'setup' requires parameters", _id.c_str());
            return false;
        }

        int pin = (*args)["pin"].as<int>();
        int channel = (*args)["channel"].as<int>();
        uint32_t frequency = (*args)["frequency"].as<uint32_t>();
        uint8_t resolutionBits = (*args)["resolutionBits"].as<uint8_t>();

        return setupMotor(pin, channel, frequency, resolutionBits);
    }
    else
    {
        ESP_LOGW(TAG, "PwmMotor [%s]: Unknown action: %s", _id.c_str(), action.c_str());
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
    doc["pin"] = _pin;
    doc["pwmChannel"] = _pwmChannel;
    doc["frequency"] = _frequency;
    doc["resolutionBits"] = _resolutionBits;
    doc["dutyCycle"] = _currentDutyCycle;

    if (_isAnimating)
    {
        doc["targetDutyCycle"] = _targetDutyCycle;
        doc["targetDurationMs"] = _animationDuration - (millis() - _animationStartTime);
    }

    String result;
    serializeJson(doc, result);
    return result;
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
        setDutyCycle(_targetDutyCycle, true); // Final notify like normal setDutyCycle
        ESP_LOGI(TAG, "PwmMotor [%s]: Animation complete, final duty cycle: %.1f%%",
                 _id.c_str(), _targetDutyCycle);
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
        ESP_LOGE(TAG, "PwmMotor [%s]: Failed to set animated duty cycle: %s", _id.c_str(), esp_err_to_name(err));
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