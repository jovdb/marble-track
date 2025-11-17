#ifndef LEDC_CHANNELS_H
#define LEDC_CHANNELS_H

#include <stdint.h>

/**
 * @class LedcChannels
 * @brief Manages LEDC channel allocation for ESP32 PWM peripherals
 *
 * This class provides a centralized way to acquire and release LEDC channels
 * to prevent conflicts between different devices using PWM functionality.
 * Channels 0-15 are available on ESP32.
 */
class LedcChannels {
public:
    /**
     * @brief Acquire a specific LEDC channel
     * @param channel The channel number to acquire (0-15)
     * @return true if channel was successfully acquired, false if already in use or invalid
     */
    static bool acquireSpecific(int channel);

    /**
     * @brief Acquire the first available free LEDC channel
     * @return The channel number if successful, -1 if no channels available
     */
    static int acquireFree();

    /**
     * @brief Release a previously acquired LEDC channel
     * @param channel The channel number to release
     */
    static void release(int channel);

    /**
     * @brief Check if a channel is currently in use
     * @param channel The channel number to check
     * @return true if channel is in use, false otherwise
     */
    static bool isInUse(int channel);

private:
    // Bitmask to track channel usage (16 bits for channels 0-15)
    static uint16_t channelMask;

    // Private constructor to prevent instantiation
    LedcChannels() {}
};

#endif // LEDC_CHANNELS_H