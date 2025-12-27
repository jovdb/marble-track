/**
 * @file RtosMixin.h
 * @brief RTOS task support mixin using CRTP
 *
 * Adds FreeRTOS task management to any device class.
 * Uses Curiously Recurring Template Pattern (CRTP) for static polymorphism.
 *
 * Usage:
 *   class MyDevice : public DeviceBase, public RtosMixin<MyDevice> {
 *       void task() override { ... }
 *   };
 */

#ifndef RTOS_MIXIN_H
#define RTOS_MIXIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

/**
 * @class RtosMixin
 * @brief Mixin that adds RTOS task capabilities
 * @tparam Derived The derived class (CRTP pattern)
 */
template <typename Derived>
class RtosMixin
{
public:
    RtosMixin()
    {
        // Register this mixin with the base class
        static_cast<Derived *>(this)->registerMixin("rtos");
    }

    virtual ~RtosMixin()
    {
        stopTask();
    }

    /**
     * @brief Start the RTOS task
     * @param taskName Name of the task
     * @param stackSize Stack size in bytes (default: 4096)
     * @param priority Task priority (default: 1)
     * @param core Core ID (0 or 1) (default: 1)
     * @return true if task started successfully
     */
    bool startTask(const String &taskName, uint32_t stackSize = 4096,
                   UBaseType_t priority = 1, BaseType_t core = 1)
    {
        if (_taskHandle != nullptr)
        {
            return true; // Already running
        }

        MLOG_DEBUG("%s: Starting RTOS task '%s'", static_cast<Derived *>(this)->toString().c_str(), taskName.c_str());
        BaseType_t result = xTaskCreatePinnedToCore(
            _taskTrampoline,
            taskName.c_str(),
            stackSize,
            this,
            priority,
            &_taskHandle,
            core);

        return result == pdPASS;
    }

    /**
     * @brief Stop and delete the RTOS task
     */
    void stopTask()
    {
        if (_taskHandle != nullptr)
        {
            vTaskDelete(_taskHandle);
            _taskHandle = nullptr;
        }
    }

    /**
     * @brief Suspend the RTOS task
     */
    void suspendTask()
    {
        if (_taskHandle != nullptr)
        {
            vTaskSuspend(_taskHandle);
        }
    }

    /**
     * @brief Resume the RTOS task
     */
    void resumeTask()
    {
        if (_taskHandle != nullptr)
        {
            vTaskResume(_taskHandle);
        }
    }

    /**
     * @brief Notify the task (wake it up)
     */
    void notifyTask()
    {
        if (_taskHandle != nullptr)
        {
            xTaskNotifyGive(_taskHandle);
        }
    }

    /**
     * @brief Check if task is running
     * @return true if task handle is valid
     */
    bool isTaskRunning() const
    {
        return _taskHandle != nullptr;
    }

    TaskHandle_t getTaskHandle() const { return _taskHandle; }

protected:
    TaskHandle_t _taskHandle = nullptr;

    /**
     * @brief Main task loop - must be implemented by derived class
     */
    virtual void task() = 0;

private:
    static void _taskTrampoline(void *arg)
    {
        auto *self = static_cast<RtosMixin *>(arg);
        static_cast<Derived *>(self)->task();
        // If task() returns, clean up
        self->_taskHandle = nullptr;
        vTaskDelete(nullptr);
    }
};

#endif // RTOS_MIXIN_H
