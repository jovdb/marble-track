/**
 * @file StateChangeMixin.h
 * @brief Internal state change event subscription mixin
 *
 * Adds ability for other components to subscribe to state changes.
 * This is separate from ControllableMixin's WebSocket notifications.
 *
 * Usage:
 *   class MyDevice : public DeviceBase, public StateChangeMixin<MyDevice> { ... };
 *
 *   // Subscribe to changes
 *   device.onStateChange([](void* data) { ... });
 */

#ifndef STATE_CHANGE_MIXIN_H
#define STATE_CHANGE_MIXIN_H

#include <functional>
#include <vector>

using StateChangeCallback = std::function<void(void *)>;

/**
 * @class StateChangeMixin
 * @brief Mixin that adds internal state change subscriptions
 * @tparam Derived The derived class (CRTP pattern)
 */
template <typename Derived>
class StateChangeMixin
{
public:
    StateChangeMixin()
    {
        // Register this mixin with the base class
        static_cast<Derived*>(this)->registerMixin("statechange");
    }

    virtual ~StateChangeMixin() = default;

    /**
     * @brief Subscribe to state change events
     * @param callback Function to call when state changes
     */
    void onStateChange(StateChangeCallback callback)
    {
        _stateChangeCallbacks.push_back(callback);
    }

    /**
     * @brief Clear all state change subscriptions
     */
    void clearStateChangeCallbacks()
    {
        _stateChangeCallbacks.clear();
    }

protected:
    std::vector<StateChangeCallback> _stateChangeCallbacks;

    /**
     * @brief Emit state change event to all subscribers
     * @param data Optional data pointer to pass to callbacks
     */
    void emitStateChange(void *data = nullptr)
    {
        for (auto &callback : _stateChangeCallbacks)
        {
            if (callback)
            {
                callback(data);
            }
        }
    }
};

#endif // STATE_CHANGE_MIXIN_H
