/**
 * @file Hv20tAudio.cpp
 * @brief HV20T audio module (UART mode)
 */

#include "devices/Hv20tAudio.h"
#include "Logging.h"
#include <ArduinoJson.h>

namespace devices
{
    namespace
    {
        constexpr uint8_t VOLUME_STEPS = 30;

        PinConfig parsePinConfig(const JsonVariantConst &value)
        {
            PinConfig config;
            if (value.is<int>())
            {
                config.pin = value.as<int>();
                config.expanderId = "";
                return config;
            }

            if (!value.isNull())
            {
                JsonDocument pinDoc;
                pinDoc.set(value);
                return PinFactory::jsonToConfig(pinDoc);
            }

            config.pin = -1;
            config.expanderId = "";
            return config;
        }

        uint8_t clampPercent(int value)
        {
            if (value < 0)
                return 0;
            if (value > 100)
                return 100;
            return static_cast<uint8_t>(value);
        }
    }

    Hv20tAudio::Hv20tAudio(const String &id)
        : Device(id, "hv20t"),
          _serial(2),
          _player(&_serial)
    {
    }

    Hv20tAudio::~Hv20tAudio()
    {
        if (_playerReady)
        {
            _serial.end();
        }
    }

    void Hv20tAudio::setup()
    {
        Device::setup();

        setName(_config.name);

        if (!initializePlayer())
        {
            MLOG_WARN("%s: DyPLayer not configured", toString().c_str());
        }

        _state.volumePercent = clampPercent(_config.defaultVolumePercent);
        _volumeSteps = static_cast<uint8_t>((_state.volumePercent * VOLUME_STEPS + 50) / 100);
        if (_playerReady)
        {
            setVolume(_state.volumePercent);
        }
    }

    void Hv20tAudio::teardown()
    {
        Device::teardown();

        stop();

        if (_playerReady)
        {
            _serial.end();
        }
        _playerReady = false;
        _state.isBusy = false;
        _state.lastSongIndex = -1;
        _playbackInitiated = false;
        
        // Clear any queued songs
        while (!_songQueue.empty())
        {
            _songQueue.pop();
        }
    }

    void Hv20tAudio::loop()
    {
        Device::loop();

        const bool busy = isPlaying();
        if (busy != _state.isBusy)
        {
            _state.isBusy = busy;
            notifyStateChanged();
            
            // If playback is active, mark as initiated
            if (busy)
            {
                _playbackInitiated = true;
            }
            
            // If playback just finished, check if there are queued songs
            if (!busy && _playbackInitiated)
            {
                processQueue();
                // Only reset if queue is empty (no more songs to play)
                if (_songQueue.empty())
                {
                    _playbackInitiated = false;
                }
            }
        }
    }

    std::vector<String> Hv20tAudio::getPins() const
    {
        std::vector<String> pins;
        if (_config.rxPin.pin >= 0)
            pins.push_back(_config.rxPin.toString());
        if (_config.txPin.pin >= 0)
            pins.push_back(_config.txPin.toString());
        return pins;
    }

    bool Hv20tAudio::play(int songIndex)
    {
        return play(songIndex, Hv20tPlayMode::StopThenPlay);
    }

    bool Hv20tAudio::play(int songIndex, Hv20tPlayMode mode)
    {
        if (!_playerReady)
        {
            MLOG_WARN("%s: Cannot play - DyPLayer not ready", toString().c_str());
       
            return false;
        }

        const bool isBusy = isPlaying() || _playbackInitiated;
        if (isBusy)
        {
            if (mode == Hv20tPlayMode::StopThenPlay)
            {
                MLOG_INFO("%s: Replace current song with song %i", toString().c_str(), songIndex);
                stop();
            }
            if (mode == Hv20tPlayMode::SkipIfPlaying)
            {
                MLOG_INFO("%s: Skipping play song %i - already playing", toString().c_str(), songIndex);
                return true;
            }

            if (mode == Hv20tPlayMode::QueueIfPlaying)
            {
                // Build queue representation string
                String queueStr = "[";
                if (!_songQueue.empty()) {
                    std::queue<int> tempQueue = _songQueue; // Copy queue to iterate
                    bool first = true;
                    while (!tempQueue.empty()) {
                        if (!first) queueStr += ", ";
                        queueStr += String(tempQueue.front());
                        tempQueue.pop();
                        first = false;
                    }
                }
                queueStr += "]";
                
                MLOG_INFO("%s: Queuing song %i (queue: %s)", toString().c_str(), songIndex, queueStr.c_str());
                _songQueue.push(songIndex);
                return true;
            }
        }

        if (songIndex >= 0)
        {
            if (songIndex > 65535)
                songIndex = 65535;
            MLOG_INFO("%s: Playing song %i", toString().c_str(), songIndex);
            _state.lastSongIndex = songIndex;
            notifyStateChanged();
            _playbackInitiated = true;
            _player.playSpecified(static_cast<uint16_t>(songIndex));
            return true;
        }

        return false;
    }

    bool Hv20tAudio::stop()
    {
        if (!_playerReady)
        {
            MLOG_WARN("%s: Cannot stop - DyPLayer not ready", toString().c_str());
            return false;
        }

        _player.stop();
        _playbackInitiated = false;
        
        // Clear any queued songs when stopping
        while (!_songQueue.empty())
        {
            _songQueue.pop();
        }
        
        return true;
    }

    bool Hv20tAudio::setVolume(uint8_t percent)
    {
        if (!_playerReady)
        {
            MLOG_WARN("%s: Cannot set volume - DyPLayer not ready", toString().c_str());
            return false;
        }

        const uint8_t clamped = clampPercent(percent);
        const uint8_t targetSteps = static_cast<uint8_t>((clamped * VOLUME_STEPS + 50) / 100);
        _player.setVolume(targetSteps);

        _volumeSteps = targetSteps;
        _state.volumePercent = clamped;
        notifyStateChanged();
        return true;
    }

    bool Hv20tAudio::removeFromQueue(int songIndex)
    {
        std::queue<int> tempQueue;
        bool removed = false;

        while (!_songQueue.empty())
        {
            int song = _songQueue.front();
            _songQueue.pop();
            if (!removed && song == songIndex)
            {
                removed = true;
                MLOG_INFO("%s: Removed song %i from queue", toString().c_str(), songIndex);
            }
            else
            {
                tempQueue.push(song);
            }
        }

        _songQueue = std::move(tempQueue);
        return removed;
    }

    void Hv20tAudio::addStateToJson(JsonDocument &doc)
    {
        doc["isBusy"] = _state.isBusy;
        doc["volumePercent"] = _state.volumePercent;
        doc["lastSongIndex"] = _state.lastSongIndex;
    }

    bool Hv20tAudio::control(const String &action, JsonObject *args)
    {
        if (action == "play")
        {
            int index = -1;
            Hv20tPlayMode mode = Hv20tPlayMode::StopThenPlay;
            if (args && (*args)["songIndex"].is<int>())
            {
                index = (*args)["songIndex"].as<int>();
            }
            if (args && (*args)["mode"].is<String>())
            {
                const String modeStr = (*args)["mode"].as<String>();
                if (modeStr.equalsIgnoreCase("skip"))
                {
                    mode = Hv20tPlayMode::SkipIfPlaying;
                }
                else if (modeStr.equalsIgnoreCase("queue"))
                {
                    mode = Hv20tPlayMode::QueueIfPlaying;
                }
                else if (modeStr.equalsIgnoreCase("stop"))
                {
                    mode = Hv20tPlayMode::StopThenPlay;
                }
            }
            MLOG_INFO("%s: Play action started with index %d", toString().c_str(), index);
            return play(index, mode);
        }
        if (action == "stop")
        {
            return stop();
        }
        if (action == "setVolume")
        {
            if (!args || !(*args)["percent"].is<int>())
            {
                return false;
            }
            return setVolume(static_cast<uint8_t>((*args)["percent"].as<int>()));
        }

        MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
        return false;
    }

    void Hv20tAudio::jsonToConfig(const JsonDocument &config)
    {
        if (config["name"].is<String>())
            _config.name = config["name"].as<String>();
        if (!config["rxPin"].isNull())
            _config.rxPin = parsePinConfig(config["rxPin"]);
        if (!config["txPin"].isNull())
            _config.txPin = parsePinConfig(config["txPin"]);
        if (!config["busyPin"].isNull())
            _config.busyPin = parsePinConfig(config["busyPin"]);
        if (config["defaultVolumePercent"].is<int>())
            _config.defaultVolumePercent = clampPercent(config["defaultVolumePercent"].as<int>());
    }

    void Hv20tAudio::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.rxPin, pinDoc);
            doc["rxPin"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.txPin, pinDoc);
            doc["txPin"] = pinDoc.as<JsonVariant>();
        }
        {
            JsonDocument pinDoc;
            PinFactory::configToJson(_config.busyPin, pinDoc);
            doc["busyPin"] = pinDoc.as<JsonVariant>();
        }
        doc["defaultVolumePercent"] = _config.defaultVolumePercent;
    }

    bool Hv20tAudio::initializePlayer()
    {
        if (_config.rxPin.pin < 0 || _config.txPin.pin < 0)
        {
            MLOG_WARN("%s: UART RX/TX pins not configured", toString().c_str());
            return false;
        }

        if (_config.rxPin.pin == _config.txPin.pin)
        {
            MLOG_ERROR("%s: UART RX/TX pins must be different (%d)", toString().c_str(), _config.rxPin.pin);
            return false;
        }

        if (!_config.rxPin.expanderId.isEmpty() || !_config.txPin.expanderId.isEmpty())
        {
            MLOG_WARN("%s: UART pins must be GPIO (expander not supported)", toString().c_str());
            return false;
        }

        _serial.begin(9600, SERIAL_8N1, _config.rxPin.pin, _config.txPin.pin);
        _playerReady = true;
        MLOG_INFO("%s: DyPLayer configured (RX %d, TX %d)", toString().c_str(), _config.rxPin.pin, _config.txPin.pin);
        return true;
    }

    bool Hv20tAudio::isPlaying()
    {
        if (!_playerReady)
        {
            return false;
        }


        return _player.checkPlayState() == DY::PlayState::Playing;
    }

    void Hv20tAudio::processQueue()
    {
        if (!_playerReady)
        {
            return;
        }

        if (!_songQueue.empty())
        {
            const int nextSong = _songQueue.front();
            _songQueue.pop();
            MLOG_INFO("%s: Playing next queued song %i (%zu remaining in queue)", 
                      toString().c_str(), nextSong, _songQueue.size());
            play(nextSong, Hv20tPlayMode::StopThenPlay);
        }
    }

} // namespace devices
