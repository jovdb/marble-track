/**
 * @file StateMixin.h
 * @brief Generic state management mixin with callbacks
 *
 * Provides state tracking and change notifications for any device.
 * The derived class defines its own state struct.
 *
 * Usage:
 *   struct MyState { int value; String mode; };
 *   class MyDevice : public Device, public StateMixin<MyDevice, MyState> {
 *       void onStateChange(EventCallback callback) override;
 *   };
 */

#ifndef STATE_MIXIN_H
#define STATE_MIXIN_H

#include <functional>
#include <vector>
#include <algorithm>

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
    StateMixin()
    {
        // Register this mixin with the base class
        static_cast<Derived *>(this)->registerMixin("state");
    }

    virtual ~StateMixin() = default;

    /**
     * @brief Get state (read-only reference)
     * @return Reference to current state
     */
    const StateType &getState() const
    {
        return _state;
    }

    /**
     * @brief Subscribe to state change events
     * @param callback Function to call when state changes
     * @return Function to unsubscribe from state changes
     */
    std::function<void()> onStateChange(EventCallback callback)
    {
        size_t callbackId = _nextCallbackId++;
        _stateChangeCallbacks.emplace_back(callbackId, callback);
        // Return unsubscribe function
        return [this, callbackId]() {
            auto it = std::find_if(_stateChangeCallbacks.begin(), _stateChangeCallbacks.end(),
                [callbackId](const CallbackEntry& entry) { return entry.id == callbackId; });
            if (it != _stateChangeCallbacks.end()) {
                _stateChangeCallbacks.erase(it);
            }
        };
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
        for (auto &entry : _stateChangeCallbacks)
        {
            if (entry.callback)
            {
                entry.callback(&_state);
            }
        }
    }

private:
    struct CallbackEntry {
        size_t id;
        EventCallback callback;
        CallbackEntry(size_t id, EventCallback callback) : id(id), callback(callback) {}
    };

    std::vector<CallbackEntry> _stateChangeCallbacks;
    size_t _nextCallbackId = 0;
};

#endif // STATE_MIXIN_H
