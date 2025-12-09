#ifndef TEST_TASK_DEVICE_H
#define TEST_TASK_DEVICE_H

#include "devices/ControllableTaskDevice.h"
#include "devices/ButtonDevice.h"

class TestTaskDevice : public ControllableTaskDevice
{
public:
    TestTaskDevice(const String &id, ButtonDevice *button, NotifyClients callback = nullptr);

    void getConfigFromJson(const JsonDocument &config) override;
    void addConfigToJson(JsonDocument &doc) const override;

protected:
    void task() override;

private:
    ButtonDevice *_button;
    bool _lastPressed = false;
};

#endif // TEST_TASK_DEVICE_H