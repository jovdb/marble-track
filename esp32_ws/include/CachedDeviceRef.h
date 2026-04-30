#pragma once

#include <Arduino.h>
#include "DeviceManager.h"

/**
 * @brief A self-invalidating cached pointer to a device owned by DeviceManager.
 *
 * Holds a lazily-resolved pointer to a device looked up by id. Subscribes to
 * DeviceManager::addOnDevicesChanged() at construction and clears the cached
 * pointer when the device tree changes, so the next access re-fetches a fresh
 * pointer from the manager.
 *
 * Replaces the older pattern of:
 *   - declaring a global raw pointer
 *   - manually nulling it from a setOnDevicesChanged() callback
 *   - re-fetching it every loop iteration
 *
 * The wrapper owns no devices: the underlying object is always owned by the
 * DeviceManager. Never delete the pointer obtained from this wrapper.
 *
 * Usage:
 *   CachedDeviceRef<MarbleController> controller(deviceManager, "controller");
 *   if (auto *c = controller.get()) c->doSomething();
 *   controller->doSomething();   // operator-> + implicit get()
 *   if (controller) { ... }      // bool conversion
 */
template <typename T>
class CachedDeviceRef
{
public:
    CachedDeviceRef(DeviceManager &manager, const String &deviceId)
        : _manager(manager), _id(deviceId), _ptr(nullptr)
    {
        // Self-invalidate on every device tree change.
        _manager.addOnDevicesChanged([this]() { _ptr = nullptr; });
    }

    // Non-copyable: each instance owns a subscription that captures `this`.
    CachedDeviceRef(const CachedDeviceRef &) = delete;
    CachedDeviceRef &operator=(const CachedDeviceRef &) = delete;

    /**
     * @brief Resolve and return the device pointer (re-fetches if needed).
     * @return Pointer to the device, or nullptr if not present in the tree.
     */
    T *get()
    {
        if (!_ptr)
        {
            _ptr = _manager.getDeviceByIdAs<T>(_id);
        }
        return _ptr;
    }

    T *operator->() { return get(); }
    explicit operator bool() { return get() != nullptr; }

private:
    DeviceManager &_manager;
    String _id;
    T *_ptr;
};
