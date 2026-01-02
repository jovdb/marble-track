/**
 * @file Buzzer.h
 * @brief Buzzer device using Device with composition mixins
 */

#ifndef COMPOSITION_BUZZER_H
#define COMPOSITION_BUZZER_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/mixins/RtosMixin.h"
#include "LedcChannels.h"
#include <freertos/semphr.h>

namespace devices
{

    /**
     * @struct BuzzerConfig
     * @brief Configuration for Buzzer device
     */
    struct BuzzerConfig
    {
        int pin = -1;           // GPIO pin number (-1 = not configured)
        String name = "Buzzer"; // Device name
    };

    /**
     * @struct ToneCommand
     * @brief Inter-thread communication for tone requests
     */
    struct ToneCommand
    {
        bool pending = false;      // True if a tone request is waiting
        int frequency = 0;         // Frequency in Hz for pending tone
        int duration = 0;          // Duration in ms for pending tone
    };

    /**
     * @struct TuneCommand
     * @brief Inter-thread communication for tune requests
     */
    struct TuneCommand
    {
        bool pending = false;      // True if a tune request is waiting
        String rtttl = "";         // RTTTL string for pending tune
    };

    /**
     * @struct BuzzerState
     * @brief State structure for Buzzer device
     */
    struct BuzzerState
    {
        String mode = "IDLE"; // IDLE, TONE, or TUNE
        unsigned long playStartTime = 0;
        unsigned long toneDuration = 0;
        String currentTune = "";
        bool stopRequested = false; // Flag to interrupt current playback
        ToneCommand toneCommand; // Inter-thread tone communication
        TuneCommand tuneCommand; // Inter-thread tune communication
    };

    /**
     * @class Buzzer
     * @brief Buzzer with configurable pin, tone/tune playback, and control interface
     */
    class Buzzer : public Device,
                   public ConfigMixin<Buzzer, BuzzerConfig>,
                   public StateMixin<Buzzer, BuzzerState>,
                   public ControllableMixin<Buzzer>,
                   public SerializableMixin<Buzzer>,
                   public RtosMixin<Buzzer>
    {
    public:
        explicit Buzzer(const String &id);
        ~Buzzer();

        void setup() override;
        void loop() override;
        std::vector<int> getPins() const override;

        /**
         * @brief Play a tone with specified frequency and duration
         * @param frequency Frequency in Hz (20-20000)
         * @param duration Duration in milliseconds
         * @return true if tone started, false if not configured
         */
        bool tone(int frequency, int duration);

        /**
         * @brief Stop any current playback
         * @return true if stopped, false if not playing
         */
        bool stop();

        /**
         * @brief Play a tune from RTTTL string
         * @param rtttl RTTTL (Ring Tone Text Transfer Language) string
         * @return true if tune started, false if not configured
         */
        bool tune(const String &rtttl);

        // ControllableMixin implementation
        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

        // RTOS task implementation
        void task() override;

        // Plotting
        void plotState() override;

    private:
        int _ledcChannel = -1; // LEDC channel assigned to this buzzer
        SemaphoreHandle_t _stateMutex; // Mutex for thread-safe state access
    };

} // namespace devices

#endif // COMPOSITION_BUZZER_H
