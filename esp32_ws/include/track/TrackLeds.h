#ifndef TRACKLEDS_H
#define TRACKLEDS_H

#include "devices/Led.h"

namespace devices
{
    class TrackLeds
    {
    public:
        TrackLeds(Led &liftLed, Led &wheelLed, Led &launcherLed, Led &spiralLed);

        void blinkBusy(Led *ledDevice) const;
        void blinkError(Led *ledDevice) const;
        void blinkInit(Led *ledDevice) const;
        void blinkAttention(Led *ledDevice) const;
        void blinkCount(Led *ledDevice, int count) const;
        int blinkLoopAll() const;

    private:
        Led *_liftLed;
        Led *_wheelLed;
        Led *_launcherLed;
        Led *_spiralLed;
    };
}

#endif // TRACKLEDS_H