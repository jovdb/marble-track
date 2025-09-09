#include "devices/PwmMotor.h"
#include "esp_log.h"

static const char *TAG = "PwmMotor";

PwmMotor::PwmMotor(const String &id, const String &name)
    : Device(id, name, "pwmmotor"), 
      _pin(-1), 
      _pwmChannel(0), 
      _frequency(1000), 
      _resolutionBits(10),
      _currentDutyCycle(0.0),
      _isSetup(false),
      _mcpwmUnit(MCPWM_UNIT_0),
      _mcpwmTimer(MCPWM_TIMER_0),
      _mcpwmSignal(MCPWM0A)
{
}

bool PwmMotor::setupMotor(int pin, int pwmChannel, uint32_t frequency, uint8_t resolutionBits)
{
    _pin = pin;
    _pwmChannel = pwmChannel;
    _frequency = frequency;
    _resolutionBits = resolutionBits;

    // Determine MCPWM unit and timer based on channel
    if (_pwmChannel == 0) {
        _mcpwmUnit = MCPWM_UNIT_0;
        _mcpwmTimer = MCPWM_TIMER_0;
        _mcpwmSignal = MCPWM0A;
    } else if (_pwmChannel == 1) {
        _mcpwmUnit = MCPWM_UNIT_0;
        _mcpwmTimer = MCPWM_TIMER_1;
        _mcpwmSignal = MCPWM1A;
    } else {
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
        .cmpr_a = 0.0,  // Start with 0% duty cycle
        .cmpr_b = 0.0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_UP_COUNTER,
    };
    
    esp_err_t err = mcpwm_init(_mcpwmUnit, _mcpwmTimer, &pwm_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PwmMotor [%s]: MCPWM initialization failed: %s", _id.c_str(), esp_err_to_name(err));
        return false;
    }

    _isSetup = true;
    ESP_LOGI(TAG, "PwmMotor [%s]: MCPWM configured successfully on pin %d", _id.c_str(), _pin);
    return true;
}

void PwmMotor::setDutyCycle(float dutyCycle)
{
    if (!_isSetup) {
        ESP_LOGE(TAG, "PwmMotor [%s]: Not setup. Call setupMotor() first.", _id.c_str());
        return;
    }

    // Clamp duty cycle to valid range
    if (dutyCycle < 0.0) dutyCycle = 0.0;
    if (dutyCycle > 100.0) dutyCycle = 100.0;

    _currentDutyCycle = dutyCycle;

    // Set the duty cycle using MCPWM
    esp_err_t err = mcpwm_set_duty(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, dutyCycle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PwmMotor [%s]: Failed to set duty cycle: %s", _id.c_str(), esp_err_to_name(err));
        return;
    }

    // Update duty cycle type to percentage
    mcpwm_set_duty_type(_mcpwmUnit, _mcpwmTimer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    ESP_LOGD(TAG, "PwmMotor [%s]: Duty cycle set to %.1f%%", _id.c_str(), dutyCycle);
    
    // Notify state change for WebSocket updates
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
    
    if (_pin == -1) {
        ESP_LOGW(TAG, "PwmMotor [%s]: No pin configured. Use setupMotor() to configure.", _id.c_str());
        return;
    }

    // If motor was already configured via setupMotor(), just ensure it's ready
    if (!_isSetup) {
        configureMCPWM();
    }
    
    ESP_LOGI(TAG, "PwmMotor [%s]: Device setup complete", _id.c_str());
}

void PwmMotor::loop()
{
    Device::loop();
    // No continuous processing needed for PWM motor
}

bool PwmMotor::control(const String &action, JsonObject *args)
{
    if (action == "setDutyCycle") {
        if (!args || !(*args)["value"].is<float>()) {
            ESP_LOGE(TAG, "PwmMotor [%s]: Invalid 'setDutyCycle' payload", _id.c_str());
            return false;
        }
        float dutyCycle = (*args)["value"].as<float>();
        setDutyCycle(dutyCycle);
        return true;
    }
    else if (action == "stop") {
        stop();
        return true;
    }
    else if (action == "setup") {
        if (!args) {
            ESP_LOGE(TAG, "PwmMotor [%s]: 'setup' requires parameters", _id.c_str());
            return false;
        }
        
        int pin = (*args)["pin"].as<int>();
        int channel = (*args)["channel"].as<int>();
        uint32_t frequency = (*args)["frequency"].as<uint32_t>();
        uint8_t resolutionBits = (*args)["resolutionBits"].as<uint8_t>();
        
        return setupMotor(pin, channel, frequency, resolutionBits);
    }
    else {
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
    for (JsonPair kv : baseDoc.as<JsonObject>()) {
        doc[kv.key()] = kv.value();
    }
    
    // Add PwmMotor specific state
    doc["pin"] = _pin;
    doc["pwmChannel"] = _pwmChannel;
    doc["frequency"] = _frequency;
    doc["resolutionBits"] = _resolutionBits;
    doc["dutyCycle"] = _currentDutyCycle;
    doc["isSetup"] = _isSetup;
    doc["running"] = (_currentDutyCycle > 0.0);
    
    String result;
    serializeJson(doc, result);
    return result;
}