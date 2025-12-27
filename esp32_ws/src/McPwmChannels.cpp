#include "McPwmChannels.h"
#include "Logging.h"

// Initialize static member
uint8_t McPwmChannels::channelMask = 0;

bool McPwmChannels::acquireSpecific(int channel)
{
    if (channel < 0 || channel >= MCPWM_SIGNAL_COUNT)
    {
        MLOG_ERROR("McPwmChannels: Invalid channel %d (must be 0-%d)", channel, MCPWM_SIGNAL_COUNT - 1);
        return false;
    }

    uint8_t mask = 1 << channel;
    if (channelMask & mask)
    {
        MLOG_WARN("McPwmChannels: Channel %d already in use", channel);
        return false;
    }

    channelMask |= mask;
    MLOG_DEBUG("McPwmChannels: Acquired channel %d", channel);
    return true;
}

int McPwmChannels::acquireFree()
{
    for (int channel = 0; channel < MCPWM_SIGNAL_COUNT; ++channel)
    {
        uint8_t mask = 1 << channel;
        if (!(channelMask & mask))
        {
            channelMask |= mask;
            MLOG_DEBUG("McPwmChannels: Acquired free channel %d", channel);
            return channel;
        }
    }
    MLOG_ERROR("McPwmChannels: No free channels available");
    return -1;
}

void McPwmChannels::release(int channel)
{
    if (channel < 0 || channel >= MCPWM_SIGNAL_COUNT)
    {
        MLOG_ERROR("McPwmChannels: Invalid channel %d (must be 0-%d)", channel, MCPWM_SIGNAL_COUNT - 1);
        return;
    }

    uint8_t mask = 1 << channel;
    if (!(channelMask & mask))
    {
        MLOG_WARN("McPwmChannels: Channel %d was not in use", channel);
        return;
    }

    channelMask &= ~mask;
    MLOG_DEBUG("McPwmChannels: Released channel %d", channel);
}

bool McPwmChannels::isInUse(int channel)
{
    if (channel < 0 || channel >= MCPWM_SIGNAL_COUNT)
    {
        return false;
    }
    uint8_t mask = 1 << channel;
    return (channelMask & mask) != 0;
}

mcpwm_io_signals_t McPwmChannels::getSignal(int channel)
{
    switch (channel)
    {
    case MCPWM0_OUT0A:
        return MCPWM0A;
    case MCPWM0_OUT0B:
        return MCPWM0B;
    case MCPWM0_OUT1A:
        return MCPWM1A;
    case MCPWM0_OUT1B:
        return MCPWM1B;
    case MCPWM0_OUT2A:
        return MCPWM2A;
    case MCPWM0_OUT2B:
        return MCPWM2B;
    default:
        MLOG_ERROR("McPwmChannels: Invalid channel %d for getSignal", channel);
        return MCPWM0A; // Default fallback
    }
}