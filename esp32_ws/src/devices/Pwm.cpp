#include "devices/PwmDevice.h"
#include "Logging.h"
#include <ArduinoJson.h>

PwmDevice::PwmDevice(const String &id, const String &name)
    : Device(id, "pwm"),
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

bool PwmDevice::setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits)
{
    if (pin < 0)
    {
        MLOG_ERROR("Pwm [%s]: Invalid pin %d. Pin must be >= 0.", _id.c_str(), pin);
        _isSetup = false;
        _pin = -1;
        return false;
    }

    if (pwmChannel < 0 || pwmChannel > 1)
    {
        MLOG_ERROR("Pwm [%s]: Invalid PWM channel %d. Must be 0 or 1.", _id.c_str(), pwmChannel);
        _isSetup = false;
        return false;
    }

    if (frequency == 0)
    {
        MLOG_WARN("Pwm [%s]: Frequency cannot be 0. Falling back to 5000 Hz.", _id.c_str());
        frequency = 5000;
    }

    if (resolutionBits < 1 || resolutionBits > 16)
    {
        MLOG_WARN("Pwm [%s]: Resolution %d out of range. Clamping between 1 and 16.", _id.c_str(), resolutionBits);
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

    MLOG_INFO("Pwm [%s]: Setup - pin:%d, channel:%d, freq:%d Hz, resolution:%d bits",
              _id.c_str(), _pin, _pwmChannel, _frequency, _resolutionBits);

    bool configured = configureMCPWM();
    if (configured)
    {
        notifyStateChange();
    }

    return configured;
}

bool PwmDevice::configureMCPWM()
{
    if (_pin < 0)
    {
        MLOG_WARN("Pwm [%s]: Cannot configure MCPWM without a valid pin.", _id.c_str());
        _isSetup = false;
        return false;
    }

    esp_err_t gpioErr = mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _pin);
    if (gpioErr != ESP_OK)
    {
        MLOG_ERROR("Pwm [%s]: Failed to initialize MCPWM GPIO: %s", _id.c_str(), esp_err_to_name(gpioErr));
        _isSetup = false;
        return false;
    }

    mcpwm_config_t pwm_config = {
        .frequency = _frequency,
        .cmpr_a = 0.0,
        .cmpr_b = 0.0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_UP_COUNTER,
    };

    esp_err_t err = mcpwm_init(_mcpwmUnit, _mcpwmTimer, &pwm_config);
    if (err != ESP_OK)
    {
        MLOG_ERROR("Pwm [%s]: MCPWM initialization failed: %s", _id.c_str(), esp_err_to_name(err));
        _isSetup = false;
        return false;
    }

    _isSetup = true;
    MLOG_INFO("Pwm [%s]: MCPWM configured successfully on pin %d", _id.c_str(), _pin);
    return true;
}

void PwmDevice::setDutyCycle(float dutyCycle, bool notifyChange)
{
    if (!_isSetup)
    {
        MLOG_ERROR("Pwm [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return;
    }

    if (dutyCycle < 0.0)
        dutyCycle = 0.0;
    if (dutyCycle > 100.0)
        dutyCycle = 100.0;

    _currentDutyCycle = dutyCycle;

    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, dutyCycle);
    if (err != ESP_OK)
    {
        MLOG_ERROR("Pwm [%s]: Failed to set duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        return;
    }

    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    MLOG_INFO("Pwm [%s]: Duty cycle set to %.1f%%", _id.c_str(), dutyCycle);

    if (notifyChange)
    {
        notifyStateChange();
    }
}

void PwmDevice::setDutyCycleAnimated(float dutyCycle, uint32_t durationMs)
{
    if (!_isSetup)
    {
        MLOG_ERROR("Pwm [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return;
    }

    if (dutyCycle < 0.0)
        dutyCycle = 0.0;
    if (dutyCycle > 100.0)
        dutyCycle = 100.0;

    if (durationMs == 0)
    {
        setDutyCycle(dutyCycle);
        return;
    }

    _startDutyCycle = _currentDutyCycle;
    _targetDutyCycle = dutyCycle;
    _animationStartTime = millis();
    _animationDuration = durationMs;
    _isAnimating = true;

    MLOG_INFO("Pwm [%s]: Starting animated transition from %.1f%% to %.1f%% over %dms",
              _id.c_str(), _startDutyCycle, _targetDutyCycle, durationMs);

    notifyStateChange();
}

void PwmDevice::stop()
{
    setDutyCycle(0.0);
    MLOG_INFO("Pwm [%s]: Output stopped", _id.c_str());
}

void PwmDevice::setup()
{
    Device::setup();

    if (_pin == -1)
    {
        return;
    }

    if (!_isSetup)
    {
        configureMCPWM();
    }
}

void PwmDevice::loop()
{
    Device::loop();

    if (_isAnimating)
    {
        updateAnimation();
    }
}

bool PwmDevice::control(const String &action, JsonObject *args)
{
    if (action == "setDutyCycle")
    {
        if (!args || !(*args)["value"].is<float>())
        {
            MLOG_ERROR("Pwm [%s]: Invalid 'setDutyCycle' payload", _id.c_str());
            return false;
        }
        float dutyCycle = (*args)["value"].as<float>();

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
            MLOG_ERROR("Pwm [%s]: 'setup' requires parameters", _id.c_str());
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
        MLOG_WARN("Pwm [%s]: Unknown action: %s", _id.c_str(), action.c_str());
        return false;
    }
}

String PwmDevice::getState()
{
    JsonDocument doc;

    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }

    doc["pin"] = _pin;
    doc["pwmChannel"] = _pwmChannel;
    doc["frequency"] = _frequency;
    doc["resolutionBits"] = _resolutionBits;
    doc["dutyCycle"] = _currentDutyCycle;
    doc["running"] = (_currentDutyCycle > 0.0f) || _isAnimating;

    if (_isAnimating)
    {
        doc["targetDutyCycle"] = _targetDutyCycle;
        doc["targetDurationMs"] = _animationDuration - (millis() - _animationStartTime);
    }

    String result;
    serializeJson(doc, result);
    return result;
}

String PwmDevice::getConfig() const
{
    JsonDocument config;
    deserializeJson(config, Device::getConfig());

    config["name"] = _name;
    config["pin"] = _pin;
    config["pwmChannel"] = _pwmChannel;
    config["frequency"] = _frequency;
    config["resolutionBits"] = _resolutionBits;

    String message;
    serializeJson(config, message);
    return message;
}

void PwmDevice::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    if (!config)
    {
        MLOG_WARN("Pwm [%s]: Null config provided", _id.c_str());
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

void PwmDevice::updateAnimation()
{
    if (!_isAnimating)
        return;

    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _animationStartTime;

    if (elapsed >= _animationDuration)
    {
        _isAnimating = false;
        setDutyCycle(_targetDutyCycle, true);
        MLOG_INFO("Pwm [%s]: Animation complete, final duty cycle: %.1f%%",
                  _id.c_str(), _targetDutyCycle);
        return;
    }

    float progress = (float)elapsed / (float)_animationDuration;
    float easedProgress = easeInOutQuad(progress);
    float currentDutyCycle = _startDutyCycle + ((_targetDutyCycle - _startDutyCycle) * easedProgress);
    _currentDutyCycle = currentDutyCycle;

    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, currentDutyCycle);
    if (err != ESP_OK)
    {
        MLOG_ERROR("Pwm [%s]: Failed to set animated duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        _isAnimating = false;
        return;
    }

    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

float PwmDevice::easeInOutQuad(float t)
{
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    if (t < 0.5f)
    {
        return 2.0f * t * t;
    }
    else
    {
        return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }
}

std::vector<int> PwmDevice::getPins() const
{
    if (_pin < 0)
    {
        return {};
    }

    return {_pin};
}
