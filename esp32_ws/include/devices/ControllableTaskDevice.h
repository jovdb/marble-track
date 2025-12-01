/**
 * @file ControllableTaskDevice.h
 * @brief Base class for controllable task-based devices
 */

#ifndef CONTROLLABLE_TASK_DEVICE_H
#define CONTROLLABLE_TASK_DEVICE_H

#include "devices/SaveableTaskDevice.h"
#include "devices/Device.h"
#include <ArduinoJson.h>
#include <vector>

/**
 * @class ControllableTaskDevice
 * @brief Base class for devices that run on their own RTOS task and are controllable
 *
 * This class extends SaveableTaskDevice with control and state methods.
 */
class ControllableTaskDevice : public SaveableTaskDevice
{
public:
    ControllableTaskDevice(const String &id, const String &type, NotifyClients callback = nullptr);
    virtual ~ControllableTaskDevice();

    /**
     * @brief Get the current state as a JSON document
     * @return JSON document representing the device state
     */
    JsonDocument getState();

    /**
     * @brief Add device-specific state to the JSON document
     * @param doc The JSON document to extend
     */
    virtual void addStateToJson(JsonDocument &doc);

    /**
     * @brief Control the device with an action and optional arguments
     * @param action The action to perform
     * @param args Optional JSON object with arguments
     * @return true if the action was successful
     */
    virtual bool control(const String &action, JsonObject *args = nullptr);

    /**
     * @brief Get the pins used by this device
     * @return Vector of pin numbers
     */
    virtual std::vector<int> getPins() const;

    /**
     * @brief Check if this device is controllable
     * @return true if the device is controllable
     */
    virtual bool isControllable() const override { return true; }
    void notifyClients(JsonDocument &doc);
    void notifyState(bool changed);
    void notifyConfig(bool changed);

protected:
    NotifyClients _notifyClients;
};

#endif // CONTROLLABLE_TASK_DEVICE_H