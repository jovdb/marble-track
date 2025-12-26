/**
 * @file Buzzer.h
 * @brief Buzzer device using DeviceBase with composition mixins
 */

#ifndef COMPOSITION_BUZZER_H
#define COMPOSITION_BUZZER_H

#include "devices/composition/DeviceBase.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "LedcChannels.h"

namespace composition
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
     * @struct BuzzerState
     * @brief State structure for Buzzer device
     */
    struct BuzzerState
    {
        String mode = "IDLE"; // IDLE, TONE, or TUNE
        unsigned long playStartTime = 0;
        unsigned long toneDuration = 0;
        String currentTune = "";
    };

    /**
     * @class Buzzer
     * @brief Buzzer with configurable pin, tone/tune playback, and control interface
     */
    class Buzzer : public DeviceBase,
                   public ConfigMixin<Buzzer, BuzzerConfig>,
                   public StateMixin<Buzzer, BuzzerState>,
                   public ControllableMixin<Buzzer>,
                   public SerializableMixin<Buzzer>
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

    private:
        int _ledcChannel = -1; // LEDC channel assigned to this buzzer
    };

} // namespace composition

#endif // COMPOSITION_BUZZER_H
