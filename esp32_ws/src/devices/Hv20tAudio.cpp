/**
 * @file Hv20tAudio.cpp
 * @brief HV20T audio module (UART mode)
 */

#include "devices/Hv20tAudio.h"
#include "Logging.h"
#include <ArduinoJson.h>
#include <deque>

namespace devices
{
    namespace
    {
        constexpr uint8_t HV20T_START_BYTE = 0xAA;
        constexpr uint8_t CMD_PLAY = 0x02;
        constexpr uint8_t CMD_STOP = 0x04;
        constexpr uint8_t CMD_VOLUME_UP = 0x14;
        constexpr uint8_t CMD_VOLUME_DOWN = 0x15;
        constexpr uint8_t CMD_PLAY_INDEX = 0x07; // Assumed play-by-index command
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
          _serial(2)
    {
    }

    Hv20tAudio::~Hv20tAudio()
    {
        cleanupPins();
    }

    void Hv20tAudio::setup()
    {
        Device::setup();

        setName(_config.name);

        cleanupPins();

        if (!initializeSerial())
        {
            MLOG_WARN("%s: UART not configured", toString().c_str());
        }

        initializeBusyPin();

        _state.volumePercent = clampPercent(_config.defaultVolumePercent);
        _volumeSteps = static_cast<uint8_t>((_state.volumePercent * VOLUME_STEPS + 50) / 100);
        if (_serialReady)
        {
            setVolume(_state.volumePercent);
        }
    }

    void Hv20tAudio::loop()
    {
        Device::loop();

        if (_busyPin && _busyPin->isConfigured())
        {
            const int value = _busyPin->read();
            if (value >= 0)
            {
                const bool busy = value == HIGH;
                if (busy != _state.isBusy)
                {
                    _state.isBusy = busy;
                    notifyStateChanged();
                }
            }
        }

        if (!_state.isBusy && !_playQueue.empty())
        {
            const int nextSong = _playQueue.front();
            _playQueue.pop_front();
            sendPlayCommand(nextSong);
        }
    }

    std::vector<String> Hv20tAudio::getPins() const
    {
        std::vector<String> pins;
        if (_config.rxPin.pin >= 0)
            pins.push_back(_config.rxPin.toString());
        if (_config.txPin.pin >= 0)
            pins.push_back(_config.txPin.toString());
        if (_busyPin && _busyPin->isConfigured())
            pins.push_back(_busyPin->toString());
        return pins;
    }

    bool Hv20tAudio::play(int songIndex)
    {
        return play(songIndex, Hv20tPlayMode::StopThenPlay);
    }

    bool Hv20tAudio::play(int songIndex, Hv20tPlayMode mode)
    {
        if (!_serialReady)
        {
            MLOG_WARN("%s: Cannot play - UART not ready", toString().c_str());
            return false;
        }

        const bool canDetectBusy = _busyPin && _busyPin->isConfigured();
        const bool isBusy = canDetectBusy ? _state.isBusy : false;

        if (isBusy)
        {
            if (mode == Hv20tPlayMode::SkipIfPlaying)
            {
                return false;
            }
            if (mode == Hv20tPlayMode::QueueIfPlaying)
            {
                if (_playQueue.size() < 10)
                {
                    _playQueue.push_back(songIndex);
                    return true;
                }
                return false;
            }

            stop();
        }

        return sendPlayCommand(songIndex);
    }

    bool Hv20tAudio::stop()
    {
        if (!_serialReady)
        {
            MLOG_WARN("%s: Cannot stop - UART not ready", toString().c_str());
            return false;
        }
        return sendCommand(CMD_STOP, 0);
    }

    bool Hv20tAudio::setVolume(uint8_t percent)
    {
        if (!_serialReady)
        {
            MLOG_WARN("%s: Cannot set volume - UART not ready", toString().c_str());
            return false;
        }

        const uint8_t clamped = clampPercent(percent);
        const uint8_t targetSteps = static_cast<uint8_t>((clamped * VOLUME_STEPS + 50) / 100);
        const int diff = static_cast<int>(targetSteps) - static_cast<int>(_volumeSteps);

        if (diff > 0)
        {
            for (int i = 0; i < diff; i++)
            {
                sendCommand(CMD_VOLUME_UP, 0);
                delay(5);
            }
        }
        else if (diff < 0)
        {
            for (int i = 0; i < -diff; i++)
            {
                sendCommand(CMD_VOLUME_DOWN, 0);
                delay(5);
            }
        }

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

    bool Hv20tAudio::initializeSerial()
    {
        if (_config.rxPin.pin < 0 || _config.txPin.pin < 0)
        {
            return false;
        }

        if (!_config.rxPin.expanderId.isEmpty() || !_config.txPin.expanderId.isEmpty())
        {
            MLOG_WARN("%s: UART pins must be GPIO (expander not supported)", toString().c_str());
            return false;
        }

        _serial.begin(9600, SERIAL_8N1, _config.rxPin.pin, _config.txPin.pin);
        _serialReady = true;
        return true;
    }

    bool Hv20tAudio::initializeBusyPin()
    {
        if (_config.busyPin.pin < 0)
        {
            return false;
        }

        _busyPin = PinFactory::createPin(_config.busyPin);
        if (!_busyPin)
        {
            MLOG_ERROR("%s: Failed to create busy pin %s", toString().c_str(), _config.busyPin.toString().c_str());
            return false;
        }

        if (!_busyPin->setup(_config.busyPin.pin, pins::PinMode::Input))
        {
            MLOG_ERROR("%s: Failed to setup busy pin %s", toString().c_str(), _config.busyPin.toString().c_str());
            return false;
        }

        return true;
    }

    void Hv20tAudio::cleanupPins()
    {
        if (_busyPin)
        {
            delete _busyPin;
            _busyPin = nullptr;
        }
    }

    bool Hv20tAudio::sendPlayCommand(int songIndex)
    {
        if (songIndex < 0)
        {
            return sendCommand(CMD_PLAY, 0);
        }

        if (songIndex > 255)
        {
            songIndex = 255;
        }

        _state.lastSongIndex = songIndex;
        notifyStateChanged();
        return sendCommand(CMD_PLAY_INDEX, static_cast<uint8_t>(songIndex));
    }

    bool Hv20tAudio::sendCommand(uint8_t command, uint8_t param)
    {
        if (!_serialReady)
        {
            return false;
        }

        const uint8_t checksum = static_cast<uint8_t>(HV20T_START_BYTE + command + param);
        uint8_t buffer[4] = {HV20T_START_BYTE, command, param, checksum};
        const size_t written = _serial.write(buffer, sizeof(buffer));
        _serial.flush();
        return written == sizeof(buffer);
    }

} // namespace devices
