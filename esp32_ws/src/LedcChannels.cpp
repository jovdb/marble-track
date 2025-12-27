#include "LedcChannels.h"
#include "Logging.h"

// Initialize static member
uint16_t LedcChannels::channelMask = 0;

bool LedcChannels::acquireSpecific(int channel)
{
    if (channel < 0 || channel > 15)
    {
        MLOG_ERROR("LedcChannels: Invalid channel %d (must be 0-15)", channel);
        return false;
    }

    uint16_t mask = 1 << channel;
    if (channelMask & mask)
    {
        MLOG_WARN("LedcChannels: Channel %d already in use", channel);
        return false;
    }

    channelMask |= mask;
    MLOG_DEBUG("LedcChannels: Acquired channel %d", channel);
    return true;
}

int LedcChannels::acquireFree()
{
    for (int channel = 0; channel < 16; ++channel)
    {
        uint16_t mask = 1 << channel;
        if (!(channelMask & mask))
        {
            channelMask |= mask;
            MLOG_DEBUG("LedcChannels: Acquired free channel %d", channel);
            return channel;
        }
    }
    MLOG_ERROR("LedcChannels: No free channels available");
    return -1;
}

void LedcChannels::release(int channel)
{
    if (channel < 0 || channel > 15)
    {
        MLOG_ERROR("LedcChannels: Invalid channel %d (must be 0-15)", channel);
        return;
    }

    uint16_t mask = 1 << channel;
    if (!(channelMask & mask))
    {
        MLOG_WARN("LedcChannels: Channel %d was not in use", channel);
        return;
    }

    channelMask &= ~mask;
    MLOG_DEBUG("LedcChannels: Released channel %d", channel);
}

bool LedcChannels::isInUse(int channel)
{
    if (channel < 0 || channel > 15)
    {
        return false;
    }
    uint16_t mask = 1 << channel;
    return (channelMask & mask) != 0;
}