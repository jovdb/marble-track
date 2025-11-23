/**
 * @file TaskDevice.h
 * @brief Base class for task-based devices
 */

#ifndef TASK_DEVICE_H
#define TASK_DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @class TaskDevice
 * @brief Base class for devices that run on their own RTOS task
 *
 * This class manages the lifecycle of a FreeRTOS task.
 * It does NOT inherit from Device.
 */
class TaskDevice
{
public:
    TaskDevice(const String &id, const String &type);
    virtual ~TaskDevice();

    /**
     * @brief Setup and start the device task
     * @param taskName Name of the task
     * @param stackSize Stack size in bytes (default: 2048)
     * @param priority Task priority (default: 1)
     * @param core Core ID (0 or 1) (default: 1)
     * @return true if task started successfully or was already running
     */
    virtual bool setup(const String &taskName, uint32_t stackSize = 2048, UBaseType_t priority = 1, BaseType_t core = 1);

    String getId() const { return _id; }
    String getType() const { return _type; }
    String toString() const;

protected:
    String _id;
    String _type;
    TaskHandle_t _taskHandle = nullptr;

    /**
     * @brief Main task loop
     * Derived classes must implement this.
     */
    virtual void task() = 0;

private:
    static void _taskTrampoline(void *arg);
};

#endif // TASK_DEVICE_H
