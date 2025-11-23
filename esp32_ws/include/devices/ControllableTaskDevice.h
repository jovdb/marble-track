/**
 * @file ControllableTaskDevice.h
 * @brief Base class for controllable task-based devices
 */

#ifndef CONTROLLABLE_TASK_DEVICE_H
#define CONTROLLABLE_TASK_DEVICE_H

#include "devices/TaskDevice.h"
#include <ArduinoJson.h>

/**
 * @class ControllableTaskDevice
 * @brief Base class for devices that run on their own RTOS task and are controllable
 *
 * This class extends TaskDevice with control and state methods.
 */
class ControllableTaskDevice : public TaskDevice
{
public:
    ControllableTaskDevice(const String &id, const String &type);
    virtual ~ControllableTaskDevice();

    /**
     * @brief Get the current state as a JSON string
     * @return JSON string representing the device state
     */
    String getState();

    /**
     * @brief Add device-specific state to the JSON document
     * @param doc The JSON document to extend
     */
    virtual void addToState(JsonDocument &doc);

    /**
     * @brief Control the device with an action and optional arguments
     * @param action The action to perform
     * @param args Optional JSON object with arguments
     * @return true if the action was successful
     */
    virtual bool control(const String &action, JsonObject *args = nullptr);
};

#endif // CONTROLLABLE_TASK_DEVICE_H