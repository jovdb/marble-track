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
    }

    void Hv20tAudio::loop()
    {
        Device::loop();

        const bool busy = isPlaying();
        if (busy != _state.isBusy)
        {
            _state.isBusy = busy;
            notifyStateChanged();
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

        const bool isBusy = isPlaying();
        if (isBusy)
        {
            if (mode == Hv20tPlayMode::SkipIfPlaying || mode == Hv20tPlayMode::QueueIfPlaying)
            {
                return false;
            }

            stop();
        }

        if (songIndex >= 0)
        {
            if (songIndex > 65535)
                songIndex = 65535;
            _state.lastSongIndex = songIndex;
            notifyStateChanged();
            _player.playSpecified(static_cast<uint16_t>(songIndex + 1));
        }

        _player.play();
        return true;
    }

    bool Hv20tAudio::stop()
    {
        if (!_playerReady)
        {
            MLOG_WARN("%s: Cannot stop - DyPLayer not ready", toString().c_str());
            return false;
        }

        _player.stop();
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

} // namespace devices
