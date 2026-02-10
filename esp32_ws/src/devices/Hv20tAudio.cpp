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

        return; // Early return to skip device initialization and prevent hangs

        setName(_config.name);

        if (!initializePlayer())
        {
            MLOG_WARN("%s: DyPLayer not configured", toString().c_str());
        }

        if (_player.checkPlayState() == DY::PlayState::Fail)
        {
            Serial.println("PlayerState = Fail: Restart port...");
            _serial.end();
            delay(100);
            initializePlayer();
        }

        _state.volumePercent = clampPercent(_config.defaultVolumePercent);
        _state.currentPlayingSong = -1;
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
        _state.currentPlayingSong = -1;
        _currentSongStartTime = 0;

        // Clear any queued songs
        while (!_state.songQueue.empty())
        {
            _state.songQueue.pop();
        }
    }

    void Hv20tAudio::loop()
    {
        Device::loop();

        // Check for song timeout if a song is currently playing
        if (_state.currentPlayingSong >= 0 && _currentSongStartTime > 0)
        {
            unsigned long elapsed = millis() - _currentSongStartTime;
            if (elapsed >= _config.songTimeoutMs)
            {
                MLOG_WARN("%s: Song %i timed out after %lu ms, stopping playback",
                          toString().c_str(), _state.currentPlayingSong, elapsed);
                _player.stop(); // Stop the player
                // Reset software state - hardware state will be detected in busy check
                _state.currentPlayingSong = -1;
                _currentSongStartTime = 0;
                // Process queue immediately since playback has been stopped
                processQueue();
                notifyStateChanged();
            }
        }

        // Check playing state every 50ms
        if (millis() % 50 == 0)
        {
            const bool busy = isPlaying();

            // If Ended
            if (!busy && _state.currentPlayingSong >= 0)
            {
                MLOG_INFO("%s: Song %i finished playing", toString().c_str(), _state.currentPlayingSong);
                _state.currentPlayingSong = -1;
                _currentSongStartTime = 0;
                notifyStateChanged();
            }

            // If not busy and no song is playing but we have queued songs, start the next one
            // This handles the case where the device starts with a queue or queue gets populated while idle
            /*
            if (!busy && _state.currentPlayingSong == -1 && !_state.songQueue.empty())
            {
                MLOG_INFO("%s: Starting playback from queue", toString().c_str());
                processQueue();
            }*/
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

        if (_state.currentPlayingSong >= 0)
        {
            if (mode == Hv20tPlayMode::StopThenPlay)
            {
                MLOG_INFO("%s: Replace current song with song %i", toString().c_str(), songIndex);
                stop();
            }
            if (mode == Hv20tPlayMode::SkipIfPlaying)
            {
                MLOG_INFO("%s: Skipping play song %i - currently playing song %i", toString().c_str(), songIndex, _state.currentPlayingSong);
                return true;
            }

            if (mode == Hv20tPlayMode::QueueIfPlaying)
            {
                _state.songQueue.push(songIndex);
                MLOG_INFO("%s: Queuing song %i (currently playing: %i, queue: %s)", toString().c_str(), songIndex, _state.currentPlayingSong, getQueueString().c_str());
                notifyStateChanged();
                return true;
            }
        }

        if (songIndex >= 0)
        {
            if (songIndex > 65535)
                songIndex = 65535;
            MLOG_INFO("%s: Playing song %i (queue: %s)", toString().c_str(), songIndex, getQueueString().c_str());
            _state.currentPlayingSong = songIndex;
            _currentSongStartTime = millis();
            notifyStateChanged();
            _player.playSpecified(static_cast<uint16_t>(songIndex + 1));
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
        _state.currentPlayingSong = -1;
        _currentSongStartTime = 0;

        // Clear any queued songs when stopping
        while (!_state.songQueue.empty())
        {
            _state.songQueue.pop();
        }

        notifyStateChanged();
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

        while (!_state.songQueue.empty())
        {
            int song = _state.songQueue.front();
            _state.songQueue.pop();
            if (!removed && song == songIndex)
            {
                removed = true;
            }
            else
            {
                tempQueue.push(song);
            }
        }

        _state.songQueue = std::move(tempQueue);

        MLOG_INFO("%s: Removed song %i from queue (currently playing: %i, queue: %s)", toString().c_str(), songIndex, _state.currentPlayingSong, getQueueString().c_str());

        if (removed)
        {
            notifyStateChanged();
        }

        return removed;
    }

    int Hv20tAudio::getPlayingIndex()
    {
        return _state.currentPlayingSong;
    }

    void Hv20tAudio::addStateToJson(JsonDocument &doc)
    {
        doc["volumePercent"] = _state.volumePercent;
        doc["currentPlayingSong"] = _state.currentPlayingSong;
        JsonArray songQueue = doc["songQueue"].to<JsonArray>();
        std::queue<int> tempQueue = _state.songQueue; // Copy queue to iterate
        while (!tempQueue.empty())
        {
            songQueue.add(tempQueue.front());
            tempQueue.pop();
        }
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
        if (config["defaultVolumePercent"].is<int>())
            _config.defaultVolumePercent = clampPercent(config["defaultVolumePercent"].as<int>());
        if (config["songTimeoutMs"].is<unsigned long>())
            _config.songTimeoutMs = config["songTimeoutMs"].as<unsigned long>();
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
        doc["defaultVolumePercent"] = _config.defaultVolumePercent;
        doc["songTimeoutMs"] = _config.songTimeoutMs;
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

        // Clear any garbage data in the buffer
        while (_serial.available())
        {
            _serial.read();
        }

        // Give the HV20T module time to initialize after power-on/serial connection
        // Many embedded modules need some time to become responsive
        delay(50); // 1.5 second delay for module initialization

        // Initialize the DYPlayer
        _player.begin();

        _playerReady = true;
        MLOG_INFO("%s: DYPlayer configured (RX %d, TX %d)", toString().c_str(), _config.rxPin.pin, _config.txPin.pin);

        return true;
    }

    bool Hv20tAudio::isPlaying()
    {
        if (!_playerReady)
        {
            return false;
        }

        // Check software state only
        DY::PlayState playState = _player.checkPlayState();
        bool softwarePlaying = (playState == DY::PlayState::Playing);

        return softwarePlaying;
    }

    String Hv20tAudio::getQueueString()
    {
        String queueStr = "[";
        if (!_state.songQueue.empty())
        {
            std::queue<int> tempQueue = _state.songQueue; // Copy queue to iterate
            bool first = true;
            while (!tempQueue.empty())
            {
                if (!first)
                    queueStr += ", ";
                queueStr += String(tempQueue.front());
                tempQueue.pop();
                first = false;
            }
        }
        queueStr += "]";
        return queueStr;
    }

    // No notifyStateChanged call here - caller should do it if needed
    void Hv20tAudio::processQueue()
    {
        if (!_playerReady)
        {
            return;
        }

        if (!_state.songQueue.empty())
        {
            const int nextSong = _state.songQueue.front();
            _state.songQueue.pop();
            MLOG_INFO("%s: Playing next queued song %i (remaining queue: %s)",
                      toString().c_str(), nextSong, getQueueString().c_str());

            // Directly start the next song without going through busy logic
            // since we know the previous song has finished
            if (nextSong >= 0)
            {
                if (nextSong > 65535)
                {
                    // This shouldn't happen but clamp anyway
                    const int clampedSong = 65535;
                    _state.currentPlayingSong = clampedSong;
                    _player.playSpecified(static_cast<uint16_t>(clampedSong + 1));
                }
                else
                {
                    _state.currentPlayingSong = nextSong;
                    _player.playSpecified(static_cast<uint16_t>(nextSong + 1));
                }
                _currentSongStartTime = millis();
            }
            notifyStateChanged();
        }
    }

} // namespace devices
