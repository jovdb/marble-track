/**
 * @file SaveableTaskDevice.h
 * @brief TaskDevice with configuration persistence support
 */

#ifndef SAVEABLE_TASK_DEVICE_H
#define SAVEABLE_TASK_DEVICE_H

#include "devices/TaskDevice.h"

/**
 * @class SaveableTaskDevice
 * @brief Extension of TaskDevice that adds configuration save/load capabilities
 */
class SaveableTaskDevice : public TaskDevice
{
public:
    SaveableTaskDevice(const String &id, const String &type);
    virtual ~SaveableTaskDevice();

    /**
     * @brief Setup the device using a configuration object
     * @param config JSON configuration object
     * @return true if setup successful
     */
    virtual bool setup(const JsonDocument &config);

    /**
     * @brief Get current configuration
     * @return JSON document containing configuration
     */
    virtual JsonDocument getConfig() const;

    /**
     * @brief Apply configuration
     * @param config JSON configuration object
     */
    virtual void setConfig(const JsonDocument &config);

    void updateConfig(const JsonDocument &config);
};

#endif // SAVEABLE_TASK_DEVICE_H
