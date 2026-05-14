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
#include "pins/Pins.h"
#include <DYPlayerArduino.h>
#include <HardwareSerial.h>
#include <queue>

namespace devices
{

    struct Hv20tAudioConfig
    {
        String name = "HV20T";
        PinConfig rxPin;   // UART RX
        PinConfig txPin;   // UART TX
        uint8_t defaultVolumePercent = 50;
        unsigned long songTimeoutMs = 30000; // 30 seconds default timeout
    };

    struct Hv20tAudioState
    {
        uint8_t volumePercent = 50;
        int currentPlayingSong = -1;
        std::queue<int> songQueue;
    };

    enum class Hv20tPlayMode
    {
        SkipIfPlaying,
        StopThenPlay,
        QueueIfPlaying
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
        void teardown() override;
        void loop() override;
        std::vector<String> getPins() const override;

        bool play(int songIndex);
        bool play(int songIndex, Hv20tPlayMode mode);
        bool stop();
        bool setVolume(uint8_t percent);
        bool removeFromQueue(int songIndex);
        int getPlayingIndex();

        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool initializePlayer();
        void processQueue();
        String getQueueString();

        HardwareSerial _serial;
        DY::Player _player;
        bool _playerReady = false;
        uint8_t _volumeSteps = 0;
        unsigned long _currentSongStartTime = 0;

        // Async (non-blocking) song-end detection.
        // Phase 1: write the 4-byte checkPlayState command to the UART TX FIFO.
        // Phase 2: on a later loop() call, check _serial.available() >= 5 and
        //          read the already-buffered response – no waiting, no blocking.
        enum class PollState : uint8_t { Idle, Sent };
        PollState _pollState = PollState::Idle;
        unsigned long _lastPollSentMs = 0;
        unsigned long _pollSentAtMs = 0;
        static constexpr unsigned long POLL_INTERVAL_MS = 200; // how often to poll
        static constexpr unsigned long POLL_RESPONSE_WAIT_MS = 20; // give up after this
    };

} // namespace devices

#endif // COMPOSITION_HV20T_AUDIO_H
