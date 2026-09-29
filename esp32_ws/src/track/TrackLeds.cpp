#include "track/TrackLeds.h"
#include <vector>

namespace devices
{
    TrackLeds::TrackLeds(Led &liftLed, Led &wheelLed, Led &launcherLed, Led &spiralLed)
        : _liftLed(&liftLed),
          _wheelLed(&wheelLed),
          _launcherLed(&launcherLed),
          _spiralLed(&spiralLed)
    {
    }

    void TrackLeds::blinkBusy(Led *ledDevice) const
    {
        if (ledDevice)
            ledDevice->blink(480, 480);
    }

    void TrackLeds::blinkError(Led *ledDevice) const
    {
        if (ledDevice)
            ledDevice->blink(20, 940);
    }

    void TrackLeds::blinkInit(Led *ledDevice) const
    {
        if (ledDevice)
            ledDevice->blink(720, 240);
    }

    void TrackLeds::blinkAttention(Led *ledDevice) const
    {
        if (ledDevice)
            ledDevice->blink(360, 120);
    }

    void TrackLeds::blinkCount(Led *ledDevice, int count) const
    {
        if (ledDevice)
        {
            std::vector<int> pattern;
            for (auto i = 0; i < count; i++)
            {
                pattern.push_back(240);
                pattern.push_back(i == count - 1 ? 880 : 240);
            }

            ledDevice->pattern(pattern);
        }
    }

    int TrackLeds::blinkLoopAll() const
    {
        _liftLed->blink(50, 1300, 0);
        _wheelLed->blink(50, 1250, 50);
        _launcherLed->blink(50, 1200, 100);
        _spiralLed->blink(50, 1150, 150);
        return 1350;
    }
}