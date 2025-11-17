/**
 * @file Buzzer.h
 * @brief Buzzer control class for marble track system
 *
 * This class provides buzzer control functionality with tone generation and
 * RTTTL tune playback for audio feedback in the marble track system.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "Device.h"
#include "LedcChannels.h"

/**
 * @class Buzzer
 * @brief Buzzer control class with tone and tune functionality
 *
 * Provides buzzer control functionality with tone generation and RTTTL
 * (Ring Tone Text Transfer Language) tune playback capabilities.
 */
class Buzzer : public Device
{
public:
    /**
     * @brief Play a startup tone sequence
     */
    void startupTone();
    /**
     * @brief Buzzer playback modes
     */
    enum class BuzzerMode
    {
        IDLE, ///< Buzzer is not playing anything
        TONE, ///< Buzzer is playing a tone
        TUNE  ///< Buzzer is playing a tune
    };

    /**
     * @brief Constructor - creates buzzer object
     * @param id Unique identifier string for the buzzer
     */
    Buzzer(const String &id, NotifyClients callback = nullptr);

    /**
     * @brief Destructor - releases LEDC channel
     */
    ~Buzzer();

    /**
     * @brief Setup function to initialize the buzzer
     * Must be called in setup() before using the buzzer
     */
    void setup();

    void loop() override; // Handle timing for tone/tune playback

    // Controllable functionality
    bool control(const String &action, JsonObject *payload = nullptr) override;
    String getState() override;
    String getConfig() const override;
    void setConfig(JsonObject *config) override;
    std::vector<int> getPins() const override;

    // Buzzer-specific operations
    /**
     * @brief Play a tone with specified frequency and duration
     * @param frequency Frequency in Hz (20-20000)
     * @param duration Duration in milliseconds
     * @return true if tone started, false if not configured
     */
    bool tone(int frequency, int duration);

    /**
     * @brief Play a tune from RTTTL string
     * @param rtttl RTTTL (Ring Tone Text Transfer Language) string
     * @note RTTTL format: "name:settings:notes"
     * @return true if tune started, false if not configured
     */
    bool tune(const String &rtttl);

private:
    int _pin;                            ///< GPIO pin number for the buzzer
    int _ledcChannel;                    ///< LEDC channel assigned to this buzzer
    bool _isPlaying = false;             ///< Current playing state
    BuzzerMode _mode = BuzzerMode::IDLE; ///< Current playback mode
    String _currentTune = "";            ///< Currently loaded tune name

    // Timing variables
    unsigned long _playStartTime = 0; ///< When current playback started
    unsigned long _toneDuration = 0;  ///< Duration of current tone playback in ms (not used for tunes)
};

#endif // BUZZER_H