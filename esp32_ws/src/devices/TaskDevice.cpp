#include "devices/TaskDevice.h"
#include "Logging.h"

TaskDevice::TaskDevice(const String &id, const String &type) : _id(id), _type(type)
{
}

TaskDevice::~TaskDevice()
{
    if (_taskHandle != nullptr)
    {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

String TaskDevice::toString() const
{
    String upperType = _type;
    upperType.toUpperCase();
    return upperType + "[" + _id + "]";
}

bool TaskDevice::setup(const String &taskName, uint32_t stackSize, UBaseType_t priority, BaseType_t core)
{
    if (_taskHandle != nullptr)
    {
        MLOG_WARN("%s: Task already running", toString().c_str());
        return true;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        _taskTrampoline,
        taskName.c_str(),
        stackSize,
        this,
        priority,
        &_taskHandle,
        core);

    if (result != pdPASS)
    {
        MLOG_ERROR("%s: Failed to create task", toString().c_str());
        return false;
    }

    // MLOG_INFO("%s: Task started", toString().c_str());
    return true;
}

void TaskDevice::_taskTrampoline(void *arg)
{
    TaskDevice *device = static_cast<TaskDevice *>(arg);
    if (device)
    {
        device->task();
    }
    // If task returns, delete it
    vTaskDelete(NULL);
    device->_taskHandle = nullptr;
}
