/**
 * @file Buzzer.cpp
 * @brief Buzzer implementation using DeviceBase and composition mixins
 */

#include "devices/composition/Buzzer.h"
#include "Logging.h"
#include <NonBlockingRtttl.h>
#include <ArduinoJson.h>

namespace composition
{

    Buzzer::Buzzer(const String &id)
        : DeviceBase(id, "buzzer")
    {
        // Fixed channel 0 for buzzer (required by NonBlockingRTTTL library)
        // https://github.com/end2endzone/NonBlockingRTTTL/blob/master/src/NonBlockingRtttl.cpp#L90C5-L99C6
        _ledcChannel = 0;
        LedcChannels::acquireSpecific(_ledcChannel);
        
        // Create mutex for thread-safe state access
        _stateMutex = xSemaphoreCreateMutex();
    }

    Buzzer::~Buzzer()
    {
        if (_ledcChannel >= 0)
        {
            LedcChannels::release(_ledcChannel);
        }
        
        // Clean up mutex
        if (_stateMutex != nullptr)
        {
            vSemaphoreDelete(_stateMutex);
        }
    }

    void Buzzer::setup()
    {
        DeviceBase::setup();

        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured (pin = -1)", toString().c_str());
            return;
        }

        // Set the device name
        setName(_config.name);

        // Configure LEDC for tone generation
        ledcSetup(_ledcChannel, 2000, 8); // 2kHz frequency, 8-bit resolution
        ledcAttachPin(_config.pin, _ledcChannel);

        MLOG_INFO("%s: Setup on pin %d, LEDC channel %d", toString().c_str(), _config.pin, _ledcChannel);

        // Start the RTOS task for non-blocking tone/tune playback
        if (!startTask("BuzzerTask", 4096, 1, 1))
        {
            MLOG_ERROR("%s: Failed to start RTOS task", toString().c_str());
        }
        else
        {
            MLOG_INFO("%s: RTOS task started", toString().c_str());
        }
    }

    void Buzzer::loop()
    {
        DeviceBase::loop();
    }

    std::vector<int> Buzzer::getPins() const
    {
        if (_config.pin == -1)
        {
            return {};
        }
        return {_config.pin};
    }

    bool Buzzer::tone(int frequency, int duration)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured, ignoring tone command", toString().c_str());
            return false;
        }

        // Validate parameters
        if (frequency < 20 || frequency > 20000)
        {
            MLOG_ERROR("%s: Invalid frequency %dHz (range: 20-20000)", toString().c_str(), frequency);
            return false;
        }
        
        if (duration < 1 || duration > 10000)
        {
            MLOG_ERROR("%s: Invalid duration %dms (range: 1-10000)", toString().c_str(), duration);
            return false;
        }

        // Thread-safe update of tone command
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.toneCommand.pending = true;
        _state.toneCommand.frequency = frequency;
        _state.toneCommand.duration = duration;
        xSemaphoreGive(_stateMutex);

        // Wake up the RTOS task
        notifyTask();

        MLOG_INFO("%s: Queued tone %dHz for %dms", toString().c_str(), frequency, duration);
        return true;
    }

    bool Buzzer::tune(const String &rtttl)
    {
        if (_config.pin == -1)
        {
            MLOG_WARN("%s: Pin not configured, ignoring tune command", toString().c_str());
            return false;
        }

        if (rtttl.length() == 0)
        {
            MLOG_ERROR("%s: Empty RTTTL string", toString().c_str());
            return false;
        }

        // Thread-safe update of tune command
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _state.tuneCommand.pending = true;
        _state.tuneCommand.rtttl = rtttl;
        xSemaphoreGive(_stateMutex);

        // Wake up the RTOS task
        notifyTask();

        MLOG_INFO("%s: Queued RTTTL tune playback", toString().c_str());
        return true;
    }

    void Buzzer::addStateToJson(JsonDocument &doc)
    {
        // Thread-safe state read
        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {
            doc["mode"] = _state.mode;
            xSemaphoreGive(_stateMutex);
        }
    }

    bool Buzzer::control(const String &action, JsonObject *args)
    {
        if (action == "tone")
        {
            if (!args)
            {
                MLOG_ERROR("%s: 'tone' action requires args", toString().c_str());
                return false;
            }

            if (!(*args)["frequency"].is<int>())
            {
                MLOG_ERROR("%s: Invalid 'tone' args - frequency missing", toString().c_str());
                return false;
            }

            if (!(*args)["duration"].is<int>())
            {
                MLOG_ERROR("%s: Invalid 'tone' args - duration missing", toString().c_str());
                return false;
            }

            const int frequency = (*args)["frequency"].as<int>();
            const int duration = (*args)["duration"].as<int>();

            // Validate frequency range
            if (frequency < 20 || frequency > 20000)
            {
                MLOG_ERROR("%s: Invalid frequency %dHz (range: 20-20000)", toString().c_str(), frequency);
                return false;
            }

            // Validate duration
            if (duration < 1 || duration > 10000)
            {
                MLOG_ERROR("%s: Invalid duration %dms (range: 1-10000)", toString().c_str(), duration);
                return false;
            }

            return tone(frequency, duration);
        }
        else if (action == "tune")
        {
            if (!args)
            {
                MLOG_ERROR("%s: 'tune' action requires args", toString().c_str());
                return false;
            }

            if (!(*args)["rtttl"].is<String>())
            {
                MLOG_ERROR("%s: Invalid 'tune' args - need rtttl string", toString().c_str());
                return false;
            }

            const String rtttlStr = (*args)["rtttl"].as<String>();
            if (rtttlStr.length() == 0)
            {
                MLOG_ERROR("%s: Empty RTTTL string", toString().c_str());
                return false;
            }

            return tune(rtttlStr);
        }
        else
        {
            MLOG_WARN("%s: Unknown control action: %s", toString().c_str(), action.c_str());
            return false;
        }
    }

    void Buzzer::jsonToConfig(const JsonDocument &config)
    {
        if (config["pin"].is<int>())
        {
            _config.pin = config["pin"].as<int>();
        }
        if (config["name"].is<String>())
        {
            _config.name = config["name"].as<String>();
        }
    }

    void Buzzer::configToJson(JsonDocument &doc)
    {
        doc["pin"] = _config.pin;
        doc["name"] = _config.name;
    }

    /**
     * @brief RTOS task for buzzer operations
     *
     * Handles timing for tone and tune playback in a separate thread.
     */
    void Buzzer::task()
    {
        MLOG_INFO("%s: RTOS task started", toString().c_str());
        
        while (true)
        {
            // Wait for command or timeout
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
            
            // Check for pending commands
            xSemaphoreTake(_stateMutex, portMAX_DELAY);
            
            // Handle tone command
            if (_state.toneCommand.pending)
            {
                int frequency = _state.toneCommand.frequency;
                int duration = _state.toneCommand.duration;
                _state.toneCommand.pending = false;
                
                // Update state
                _state.mode = "TONE";
                _state.playStartTime = millis();
                _state.toneDuration = duration;
                xSemaphoreGive(_stateMutex);
                
                // Notify that tone has started
                notifyStateChanged();
                
                // Play the tone
                ledcWriteTone(_ledcChannel, frequency);
                vTaskDelay(pdMS_TO_TICKS(duration));
                ledcWriteTone(_ledcChannel, 0); // Stop tone
                
                // Update state back to idle
                xSemaphoreTake(_stateMutex, portMAX_DELAY);
                _state.mode = "IDLE";
                _state.playStartTime = 0;
                _state.toneDuration = 0;
                xSemaphoreGive(_stateMutex);
                
                MLOG_INFO("%s: Tone playback completed", toString().c_str());
                notifyStateChanged();
            }
            // Handle tune command
            else if (_state.tuneCommand.pending)
            {
                String rtttl = _state.tuneCommand.rtttl;
                _state.tuneCommand.pending = false;
                
                // Update state
                _state.mode = "TUNE";
                _state.currentTune = rtttl;
                xSemaphoreGive(_stateMutex);
                
                // Start the RTTTL tune
                rtttl::begin(_config.pin, rtttl.c_str());
                
                MLOG_INFO("%s: Starting RTTTL tune playback", toString().c_str());
                notifyStateChanged();
                
                // Play the tune until finished
                while (rtttl::isPlaying())
                {
                    rtttl::play();
                    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent busy waiting
                }
                
                // Tune finished
                xSemaphoreTake(_stateMutex, portMAX_DELAY);
                _state.mode = "IDLE";
                _state.currentTune = "";
                xSemaphoreGive(_stateMutex);
                
                MLOG_INFO("%s: Tune playback completed", toString().c_str());
                notifyStateChanged();
            }
            else
            {
                xSemaphoreGive(_stateMutex);
            }
        }
    }

} // namespace composition
