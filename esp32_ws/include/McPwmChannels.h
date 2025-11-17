#ifndef MCPWM_CHANNELS_H
#define MCPWM_CHANNELS_H

#include <stdint.h>
#include "driver/mcpwm.h"

/**
 * @class McPwmChannels
 * @brief Manages MCPWM channel allocation for ESP32 Motor Control PWM
 *
 * This class provides a centralized way to acquire and release MCPWM output signals
 * to prevent conflicts between different devices using MCPWM functionality.
 * Manages 6 channels: MCPWM0_OUT0A, MCPWM0_OUT0B, MCPWM0_OUT1A, MCPWM0_OUT1B, MCPWM0_OUT2A, MCPWM0_OUT2B
 */
class McPwmChannels {
public:
    // Enumeration for MCPWM output signals
    enum McPwmSignal {
        MCPWM0_OUT0A = 0,
        MCPWM0_OUT0B = 1,
        MCPWM0_OUT1A = 2,
        MCPWM0_OUT1B = 3,
        MCPWM0_OUT2A = 4,
        MCPWM0_OUT2B = 5,
        MCPWM_SIGNAL_COUNT = 6
    };

    /**
     * @brief Acquire a specific MCPWM channel
     * @param channel The channel number to acquire (0-5)
     * @return true if channel was successfully acquired, false if already in use or invalid
     */
    static bool acquireSpecific(int channel);

    /**
     * @brief Acquire the first available free MCPWM channel
     * @return The channel number if successful, -1 if no channels available
     */
    static int acquireFree();

    /**
     * @brief Release a previously acquired MCPWM channel
     * @param channel The channel number to release
     */
    static void release(int channel);

    /**
     * @brief Check if a channel is currently in use
     * @param channel The channel number to check
     * @return true if channel is in use, false otherwise
     */
    static bool isInUse(int channel);

    /**
     * @brief Get the mcpwm_io_signals_t for a channel
     * @param channel The channel number (0-5)
     * @return The corresponding mcpwm_io_signals_t, or MCPWM0A if invalid
     */
    static mcpwm_io_signals_t getSignal(int channel);

private:
    // Bitmask to track channel usage (6 bits for channels 0-5)
    static uint8_t channelMask;

    // Private constructor to prevent instantiation
    McPwmChannels() {}
};

#endif // MCPWM_CHANNELS_H