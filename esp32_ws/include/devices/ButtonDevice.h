#ifndef BUTTON_DEVICE_H
#define BUTTON_DEVICE_H

#include <atomic>
#include "devices/ControllableTaskDevice.h"

class ButtonDevice : public ControllableTaskDevice
{
public:
    enum class ButtonType
    {
        NormalOpen,
        NormalClosed
    };

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

    /**
     * @brief Get the current pressed state of the button
     * @return true if pressed, false otherwise
     */
    bool isPressed() const;
    /**
     * @brief Get the current pressed state of the button
     * @return true if released, false otherwise
     */
    bool isReleased() const;

protected:
    void task() override;

private:
    String _name;
    int _pin = -1;
    unsigned long _debounceTimeInMs = 50;
    PinModeOption _pinMode = PinModeOption::Floating;
    ButtonType _buttonType = ButtonType::NormalOpen;

    // State
    std::atomic<bool> _isPressed{false};
    std::atomic<int> _lastRawValue{0};

    // Internal for debouncing
    bool readIsButtonPressed();
    String pinModeToString(PinModeOption mode) const;
    PinModeOption pinModeFromString(const String &value) const;
    String buttonTypeToString(ButtonType type) const;
    ButtonType buttonTypeFromString(const String &value) const;
    int contactStateToPinState(bool isClosed) const;

    std::atomic<bool> _isSimulated{false};
    std::atomic<bool> _simulatedIsPressed{false};
};

#endif // BUTTON_DEVICE_H
