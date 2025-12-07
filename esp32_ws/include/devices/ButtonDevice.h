#ifndef BUTTON_DEVICE_H
#define BUTTON_DEVICE_H

#include "devices/ControllableTaskDevice.h"

class ButtonDevice : public ControllableTaskDevice
{
public:
    enum class PinModeOption
    {
        Floating,
        PullUp,
        PullDown
    };

    ButtonDevice(const String &id, NotifyClients callback = nullptr);

    void getConfigFromJson(const JsonDocument &config) override;
    void addConfigToJson(JsonDocument &doc) const override;
    void addStateToJson(JsonDocument &doc) override;
    bool control(const String &action, JsonObject *args = nullptr) override;
    std::vector<int> getPins() const override;

protected:
    void task() override;

private:
    String _name;
    int _pin = -1;
    unsigned long _debounceTimeInMs = 50;
    PinModeOption _pinMode = PinModeOption::Floating;

    // State
    volatile bool _isPressed = false;
    volatile bool _isReleased = true; // Inverse of isPressed, but kept separate if needed for logic
    
    // Internal for debouncing
    bool readIsButtonPressed();
    String pinModeToString(PinModeOption mode) const;
    PinModeOption pinModeFromString(const String &value) const;

    volatile bool _isSimulated = false;
    volatile bool _simulatedState = false;
};

#endif // BUTTON_DEVICE_H
