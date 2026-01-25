/**
 * @file Hv20tAudio.h
 * @brief HV20T audio module (UART mode)
 */

#ifndef COMPOSITION_HV20T_AUDIO_H
#define COMPOSITION_HV20T_AUDIO_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"
#include "pins/IPin.h"
#include "pins/Pins.h"
#include <HardwareSerial.h>

namespace devices
{

    struct Hv20tAudioConfig
    {
        String name = "HV20T";
        PinConfig rxPin;   // UART RX
        PinConfig txPin;   // UART TX
        PinConfig busyPin; // Busy pin
        uint8_t defaultVolumePercent = 50;
    };

    struct Hv20tAudioState
    {
        bool isBusy = false;
        uint8_t volumePercent = 50;
        int lastSongIndex = -1;
    };

    class Hv20tAudio : public Device,
                       public ConfigMixin<Hv20tAudio, Hv20tAudioConfig>,
                       public StateMixin<Hv20tAudio, Hv20tAudioState>,
                       public ControllableMixin<Hv20tAudio>,
                       public SerializableMixin<Hv20tAudio>
    {
    public:
        explicit Hv20tAudio(const String &id);
        ~Hv20tAudio() override;

        void setup() override;
        void loop() override;
        std::vector<String> getPins() const override;

        bool play(int songIndex);
        bool stop();
        bool setVolume(uint8_t percent);

        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool initializeSerial();
        bool initializeBusyPin();
        void cleanupPins();
        bool sendCommand(uint8_t command, uint8_t param = 0);

        HardwareSerial _serial;
        bool _serialReady = false;
        pins::IPin *_busyPin = nullptr;
        uint8_t _volumeSteps = 0;
    };

} // namespace devices

#endif // COMPOSITION_HV20T_AUDIO_H
