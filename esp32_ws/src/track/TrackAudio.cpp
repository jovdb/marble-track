#include "track/TrackAudio.h"
#include "SongConstants.h"
#include <utility>

namespace devices
{
    TrackAudio::TrackAudio(Hv20tAudio &audio, Buzzer &buzzer)
        : _audio(audio), _buzzer(buzzer)
    {
    }

    void TrackAudio::playStartupSound()
    {
        _audio.play(songs::STARTUP_SOUND, Hv20tPlayMode::QueueIfPlaying);
    }

    bool TrackAudio::isPlayingOneOfThese(const std::vector<int> &replaceSongIndexes) const
    {
        const auto currentIndex = _audio.getPlayingIndex();
        for (int songIndex : replaceSongIndexes)
        {
            if (currentIndex == songIndex)
            {
                return true;
            }
        }
        return false;
    }

    void TrackAudio::playErrorSound(Hv20tPlayMode mode, std::vector<int> additionalReplaceSongIndexes)
    {
        std::vector<int> replaceSongIndexes = songs::getButtonSounds();
        replaceSongIndexes.push_back(songs::ERROR);
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());

        const auto playMode = isPlayingOneOfThese(replaceSongIndexes) ? Hv20tPlayMode::StopThenPlay : mode;
        _audio.play(songs::ERROR, playMode);
    }

    void TrackAudio::playClickSound()
    {
        _buzzer.tone(640, 50);
    }

    void TrackAudio::playClickOffSound()
    {
        _buzzer.tone(320, 50);
    }

    void TrackAudio::playButtonDown(std::vector<int> additionalReplaceSongIndexes)
    {
        auto replaceSongIndexes = songs::getButtonSounds();
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());
        const auto playMode = isPlayingOneOfThese(replaceSongIndexes) ? Hv20tPlayMode::StopThenPlay : Hv20tPlayMode::SkipIfPlaying;
        _audio.play(songs::getButtonDownSound(), playMode);
    }

    void TrackAudio::playButtonUp(std::vector<int> additionalReplaceSongIndexes)
    {
        auto replaceSongIndexes = songs::getButtonSounds();
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());
        const auto playMode = isPlayingOneOfThese(replaceSongIndexes) ? Hv20tPlayMode::StopThenPlay : Hv20tPlayMode::SkipIfPlaying;
        _audio.play(songs::getButtonUpSound(), playMode);
    }

    void TrackAudio::playButtonClick(std::vector<int> additionalReplaceSongIndexes)
    {
        auto replaceSongIndexes = songs::getButtonSounds();
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());
        const auto playMode = isPlayingOneOfThese(replaceSongIndexes) ? Hv20tPlayMode::StopThenPlay : Hv20tPlayMode::SkipIfPlaying;
        _audio.play(songs::getButtonClickSound(), playMode);
    }

    void TrackAudio::playButtonCountClick(int count, std::vector<int> additionalReplaceSongIndexes)
    {
        auto replaceSongIndexes = songs::getButtonSounds();
        replaceSongIndexes.insert(replaceSongIndexes.end(), additionalReplaceSongIndexes.begin(), additionalReplaceSongIndexes.end());
        const auto playMode = isPlayingOneOfThese(replaceSongIndexes) ? Hv20tPlayMode::StopThenPlay : Hv20tPlayMode::SkipIfPlaying;
        _audio.play(songs::getButtonClickSound(count), playMode);
    }
}