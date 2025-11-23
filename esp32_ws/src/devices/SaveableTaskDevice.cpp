#include "devices/SaveableTaskDevice.h"
#include "Logging.h"

SaveableTaskDevice::SaveableTaskDevice(const String &id, const String &type)
    : TaskDevice(id, type)
{
}

SaveableTaskDevice::~SaveableTaskDevice()
{
    // Destructor implementation
}

bool SaveableTaskDevice::setup(const JsonDocument &config)
{
    setConfig(config);
    return TaskDevice::setup(_id);
}

JsonDocument SaveableTaskDevice::getConfig() const
{
    JsonDocument doc;
    doc["id"] = _id;
    doc["type"] = _type;
    return doc;
}

void SaveableTaskDevice::setConfig(const JsonDocument &config)
{
    // Default implementation: do nothing
}

/** Notify task config is changed */
void SaveableTaskDevice::updateConfig(const JsonDocument &config)
{
    // Task not started
    if (_taskHandle == nullptr)
    {
        setConfig(config);
        return;
    }

    // Suspend task while updating config
    vTaskSuspend(_taskHandle);
    setConfig(config);
    vTaskResume(_taskHandle);

    // Force task to re-read config
    xTaskNotifyGive(_taskHandle);
}
