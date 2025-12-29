#include "devices/composition/PwmMotor.h"
#include "Logging.h"

namespace composition
{

PwmMotor::PwmMotor(const String &id)
    : DeviceBase(id, "pwm-motor")
{
}

PwmMotor::~PwmMotor()
{
    if (_isSetup && _config.mcpwmChannel >= 0)
    {
        McPwmChannels::release(_config.mcpwmChannel);
    }
}

void PwmMotor::setup()
{
    DeviceBase::setup();

    if (_config.pin == -1)
    {
        MLOG_ERROR("PwmMotor [%s]: Pin not configured", getId().c_str());
        return;
    }

    if (!configureMCPWM())
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to configure MCPWM", getId().c_str());
        return;
    }

    _isSetup = true;
    MLOG_DEBUG("%s: Setup complete", toString().c_str());
}

void PwmMotor::loop()
{
    DeviceBase::loop();

    if (_isSetup && _state.isAnimating)
    {
        updateAnimation();
    }
}

std::vector<int> PwmMotor::getPins() const
{
    if (_config.pin != -1)
    {
        return {_config.pin};
    }
    return {};
}

bool PwmMotor::setDutyCycle(float dutyCycle, bool notifyChange)
{
    if (!_isSetup)
    {
        MLOG_WARN("PwmMotor [%s]: Not configured", getId().c_str());
        return false;
    }

    // Clamp duty cycle to configured range
    dutyCycle = constrain(dutyCycle, _config.minDutyCycle, _config.maxDutyCycle);

    // Set the duty cycle
    uint32_t duty = (uint32_t)((dutyCycle / 100.0f) * ((1 << _config.resolutionBits) - 1));
    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, (float)duty);
    if (err != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to set duty cycle: %s", getId().c_str(), esp_err_to_name(err));
        return false;
    }

    _state.currentDutyCycle = dutyCycle;

    if (notifyChange)
    {
        notifyStateChanged();
    }

    return true;
}

bool PwmMotor::setDutyCycleAnimated(float dutyCycle, uint32_t durationMs)
{
    if (!_isSetup)
    {
        MLOG_WARN("PwmMotor [%s]: Not configured", getId().c_str());
        return false;
    }

    // Clamp duty cycle to configured range
    dutyCycle = constrain(dutyCycle, _config.minDutyCycle, _config.maxDutyCycle);

    _state.isAnimating = true;
    _state.targetDutyCycle = dutyCycle;
    _state.animationStartTime = millis();
    _state.animationDuration = durationMs;

    return true;
}

bool PwmMotor::setValue(float value, int durationMs)
{
    // Convert normalized value (0.0-1.0) to duty cycle percentage
    float dutyCycle = _config.minDutyCycle + (value * (_config.maxDutyCycle - _config.minDutyCycle));

    if (durationMs > 0)
    {
        return setDutyCycleAnimated(dutyCycle, (uint32_t)durationMs);
    }
    else if (_config.defaultDurationInMs > 0)
    {
        return setDutyCycleAnimated(dutyCycle, _config.defaultDurationInMs);
    }
    else
    {
        return setDutyCycle(dutyCycle);
    }
}

float PwmMotor::getValue() const
{
    if (_config.maxDutyCycle == _config.minDutyCycle)
        return 0.0f;

    return (_state.currentDutyCycle - _config.minDutyCycle) / (_config.maxDutyCycle - _config.minDutyCycle);
}

void PwmMotor::stop()
{
    setDutyCycle(0.0f);
}

void PwmMotor::addStateToJson(JsonDocument &doc)
{
    doc["currentDutyCycle"] = _state.currentDutyCycle;
    doc["isAnimating"] = _state.isAnimating;
    doc["targetDutyCycle"] = _state.targetDutyCycle;
}

bool PwmMotor::control(const String &action, JsonObject *args)
{
    if (action == "setValue")
    {
        if (args && (*args)["value"].is<float>())
        {
            float value = (*args)["value"];
            int duration = -1;
            if ((*args)["duration"].is<int>())
            {
                duration = (*args)["duration"];
            }
            return setValue(value, duration);
        }
    }
    else if (action == "setDutyCycle")
    {
        if (args && (*args)["dutyCycle"].is<float>())
        {
            float dutyCycle = (*args)["dutyCycle"];
            return setDutyCycle(dutyCycle);
        }
    }
    else if (action == "stop")
    {
        stop();
        return true;
    }

    MLOG_WARN("PwmMotor [%s]: Unknown action '%s'", getId().c_str(), action.c_str());
    return false;
}

void PwmMotor::jsonToConfig(const JsonDocument &config)
{
    if (config["name"].is<String>())
    {
        _config.name = config["name"].as<String>();
    }
    if (config["pin"].is<int>())
    {
        _config.pin = config["pin"];
    }
    if (config["mcpwmChannel"].is<int>())
    {
        _config.mcpwmChannel = config["mcpwmChannel"];
    }
    if (config["frequency"].is<uint32_t>())
    {
        _config.frequency = config["frequency"];
    }
    if (config["resolutionBits"].is<uint8_t>())
    {
        _config.resolutionBits = config["resolutionBits"];
    }
    if (config["minDutyCycle"].is<float>())
    {
        _config.minDutyCycle = config["minDutyCycle"];
    }
    if (config["maxDutyCycle"].is<float>())
    {
        _config.maxDutyCycle = config["maxDutyCycle"];
    }
    if (config["defaultDurationInMs"].is<uint32_t>())
    {
        _config.defaultDurationInMs = config["defaultDurationInMs"];
    }
}

void PwmMotor::configToJson(JsonDocument &doc)
{
    doc["name"] = _config.name;
    doc["pin"] = _config.pin;
    doc["mcpwmChannel"] = _config.mcpwmChannel;
    doc["frequency"] = _config.frequency;
    doc["resolutionBits"] = _config.resolutionBits;
    doc["minDutyCycle"] = _config.minDutyCycle;
    doc["maxDutyCycle"] = _config.maxDutyCycle;
    doc["defaultDurationInMs"] = _config.defaultDurationInMs;
}

bool PwmMotor::configureMCPWM()
{
    // Acquire MCPWM channel
    if (_config.mcpwmChannel == -1)
    {
        _config.mcpwmChannel = McPwmChannels::acquireFree();
        if (_config.mcpwmChannel == -1)
        {
            MLOG_ERROR("PwmMotor [%s]: No MCPWM channels available", getId().c_str());
            return false;
        }
    }

    // Calculate MCPWM unit, timer, signal, and operator from channel index
    _mcpwmUnit = (mcpwm_unit_t)(_config.mcpwmChannel / 3);
    _mcpwmTimer = (mcpwm_timer_t)((_config.mcpwmChannel % 3));
    _mcpwmSignal = McPwmChannels::getSignal(_config.mcpwmChannel);
    _mcpwmOperator = (mcpwm_operator_t)(_config.mcpwmChannel % 3);

    // Configure MCPWM
    mcpwm_config_t pwm_config = {
        .frequency = _config.frequency,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_UP_COUNTER,
    };

    esp_err_t err = mcpwm_init(_mcpwmUnit, _mcpwmTimer, &pwm_config);
    if (err != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to init MCPWM: %s", getId().c_str(), esp_err_to_name(err));
        return false;
    }

    // Set GPIO pin
    err = mcpwm_gpio_init(_mcpwmUnit, _mcpwmSignal, _config.pin);
    if (err != ESP_OK)
    {
        MLOG_ERROR("PwmMotor [%s]: Failed to init GPIO: %s", getId().c_str(), esp_err_to_name(err));
        return false;
    }

    return true;
}

void PwmMotor::updateAnimation()
{
    uint32_t elapsed = millis() - _state.animationStartTime;
    if (elapsed >= _state.animationDuration)
    {
        // Animation complete
        setDutyCycle(_state.targetDutyCycle, true);
        _state.isAnimating = false;
    }
    else
    {
        // Calculate eased value
        float t = (float)elapsed / (float)_state.animationDuration;
        float easedT = easeInOutQuad(t);

        float currentDutyCycle = _state.currentDutyCycle +
                                easedT * (_state.targetDutyCycle - _state.currentDutyCycle);

        // Set duty cycle directly without notification to avoid spam
        uint32_t duty = (uint32_t)((currentDutyCycle / 100.0f) * ((1 << _config.resolutionBits) - 1));
        esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, _mcpwmOperator, (float)duty);
        if (err == ESP_OK)
        {
            _state.currentDutyCycle = currentDutyCycle;
        }
    }
}

float PwmMotor::easeInOutQuad(float t)
{
    return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
}

} // namespace composition