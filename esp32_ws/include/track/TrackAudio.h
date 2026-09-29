#ifndef TRACKAUDIO_H
#define TRACKAUDIO_H

#include <Arduino.h>
#include <vector>
#include "devices/Buzzer.h"
#include "devices/Hv20tAudio.h"

namespace devices
{
    struct TrackAudio
    {
        TrackAudio(Hv20tAudio &audio, Buzzer &buzzer);

        void playStartupSound();
        void playClickSound();
        void playClickOffSound();
        void playErrorSound(Hv20tPlayMode mode = Hv20tPlayMode::SkipIfPlaying, std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonDown(std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonUp(std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonClick(std::vector<int> additionalReplaceSongIndexes = {});
        void playButtonCountClick(int count, std::vector<int> additionalReplaceSongIndexes = {});

    private:
        Hv20tAudio &_audio;
        Buzzer &_buzzer;

        bool isPlayingOneOfThese(const std::vector<int> &replaceSongIndexes) const;
    };
}

#endif // TRACKAUDIO_H