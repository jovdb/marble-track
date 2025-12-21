#ifndef TEST_TASK_DEVICE_H
#define TEST_TASK_DEVICE_H

#include "devices/ControllableTaskDevice.h"
#include "devices/ButtonDevice.h"
#include "devices/LedDevice.h"

class TestTaskDevice : public ControllableTaskDevice
{
public:
    TestTaskDevice(const String &id, NotifyClients callback = nullptr);

    bool setup(const JsonDocument &config = JsonDocument()) override;
    void getConfigFromJson(const JsonDocument &config) override;
    void addConfigToJson(JsonDocument &doc) const override;

protected:
    void task() override;
    /**
     * @brief Called when a child device state changes
     */
    void update();

private:
    ButtonDevice *_button;
    LedDevice *_led;
};

#endif // TEST_TASK_DEVICE_H