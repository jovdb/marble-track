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

    // Create task with default parameters
    return TaskDevice::setup(_id);
}

JsonDocument SaveableTaskDevice::getConfig() const
{
    // Base implementation of config
    JsonDocument doc;
    doc["id"] = _id;
    doc["type"] = _type;
    addConfigToJson(doc);
    return doc;
}

void SaveableTaskDevice::addConfigToJson(JsonDocument &doc) const
{
    // Default implementation: do nothing
}

void SaveableTaskDevice::setConfig(const JsonDocument &config)
{
    // Task not started
    if (_taskHandle == nullptr)
    {
        getConfigFromJson(config);
        return;
    }

    // Suspend task while updating config
    // Restart task?
    vTaskSuspend(_taskHandle);
    getConfigFromJson(config);
    vTaskResume(_taskHandle);

    // Force task to re-read config
    xTaskNotifyGive(_taskHandle);
}

/** Extend JSON object with config information */
void SaveableTaskDevice::getConfigFromJson(const JsonDocument &config)
{
    // Default implementation: do nothing
}
