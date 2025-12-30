/**
 * @file RtosMixin.h
 * @brief RTOS task support mixin using CRTP
 *
 * Adds FreeRTOS task management to any device class.
 * Uses Curiously Recurring Template Pattern (CRTP) for static polymorphism.
 *
 * Usage:
 *   class MyDevice : public Device, public RtosMixin<MyDevice> {
 *       void task() override { ... }
 *   };
 */

#ifndef RTOS_MIXIN_H
#define RTOS_MIXIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <Arduino.h>
#include "devices/Device.h"

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
        _taskStartedSemaphore = xSemaphoreCreateBinary();
        // Register this mixin with the base class
        static_cast<Derived *>(this)->registerMixin("rtos");
    }

    virtual ~RtosMixin()
    {
        stopTask();
        if (_taskStartedSemaphore != nullptr)
        {
            vSemaphoreDelete(_taskStartedSemaphore);
        }
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

        if (result == pdPASS)
        {
            // Wait for the task to signal that it has started
            if (xSemaphoreTake(_taskStartedSemaphore, pdMS_TO_TICKS(1000)) != pdTRUE)
            {
                MLOG_ERROR("%s: Task did not start within timeout", static_cast<Derived *>(this)->toString().c_str());
                stopTask();
                return false;
            }
            return true;
        }
        return false;
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

    /**
     * @brief Override Device setup to start the RTOS task after base setup
     */
    virtual void setup()
    {
        // Call the base Device setup
        static_cast<Device *>(static_cast<Derived *>(this))->setup();
        // Start the RTOS task with default parameters
        String taskName = static_cast<Derived *>(this)->getId();
        startTask(taskName);
    }

protected:
    TaskHandle_t _taskHandle = nullptr;
    SemaphoreHandle_t _taskStartedSemaphore = nullptr;

    /**
     * @brief Main task loop - must be implemented by derived class
     */
    virtual void task() = 0;

private:
    static void _taskTrampoline(void *arg)
    {
        auto *self = static_cast<RtosMixin *>(arg);
        // Signal that the task has started
        xSemaphoreGive(self->_taskStartedSemaphore);
        static_cast<Derived *>(self)->task();
        // If task() returns, clean up
        self->_taskHandle = nullptr;
        vTaskDelete(nullptr);
    }
};

#endif // RTOS_MIXIN_H
