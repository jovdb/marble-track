/**
 * @file StateMixin.h
 * @brief Generic state management mixin with callbacks
 *
 * Provides state tracking and change notifications for any device.
 * The derived class defines its own state struct.
 *
 * Usage:
 *   struct MyState { int value; String mode; };
 *   class MyDevice : public DeviceBase, public StateMixin<MyDevice, MyState> {
 *       void onStateChange(EventCallback callback) override;
 *   };
 */

#ifndef STATE_MIXIN_H
#define STATE_MIXIN_H

#include <functional>
#include <vector>

using EventCallback = std::function<void(void *)>;

/**
 * @class StateMixin
 * @brief Mixin for generic state management with change callbacks
 * @tparam Derived The derived class (CRTP pattern)
 * @tparam StateType The state struct type for this device
 */
template <typename Derived, typename StateType>
class StateMixin
{
public:
    virtual ~StateMixin() = default;

    /**
     * @brief Get state (read-only reference)
     * @return Reference to current state
     */
    const StateType& getState() const
    {
        return _state;
    }

    /**
     * @brief Subscribe to state change events
     * @param callback Function to call when state changes
     */
    void onStateChange(EventCallback callback)
    {
        _stateChangeCallbacks.push_back(callback);
    }

protected:
    /**
     * @brief Protected state member - use getState()
     */
    StateType _state;

    /**
     * @brief Notify all subscribers that state has changed
     */
    void notifyStateChanged()
    {
        for (auto &callback : _stateChangeCallbacks)
        {
            if (callback)
            {
                callback(&_state);
            }
        }
    }

private:
    std::vector<EventCallback> _stateChangeCallbacks;

    /**
     * @brief Clear all state change subscriptions
     */
    void clearStateChangeCallbacks()
    {
        _stateChangeCallbacks.clear();
    }
};

#endif // STATE_MIXIN_H
