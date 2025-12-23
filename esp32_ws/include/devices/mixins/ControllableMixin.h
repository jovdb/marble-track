/**
 * @file ControllableMixin.h
 * @brief Mixin for devices that can report state to WebSocket clients
 *
 * Requires the derived class to implement:
 * - void addStateToJson(JsonDocument &doc);
 */

#ifndef CONTROLLABLE_MIXIN_H
#define CONTROLLABLE_MIXIN_H

#include <ArduinoJson.h>
#include "IControllable.h"

/**
 * @class ControllableMixin
 * @brief Mixin that provides state serialization capability for WebSocket control
 *
 * The derived class must implement:
 * - void addStateToJson(JsonDocument &doc) - Add device state to a JSON document
 * Automatically registers itself with DeviceBase::registerMixin("ControllableMixin")
 */
template <typename Derived>
class ControllableMixin : public IControllable
{
public:
    virtual ~ControllableMixin()
    {
        // Unregister on destruction
        auto *derived = static_cast<Derived *>(this);
        mixins::ControllableRegistry::unregisterDevice(derived->getId());
    }
    // Provide DeviceBase virtual override via mixin when combined
    virtual IControllable* getControllableInterface() { return this; }

protected:
    ControllableMixin()
    {
        // Register this mixin with the base class
        auto *derived = static_cast<Derived *>(this);
        derived->registerMixin("controllable");
        mixins::ControllableRegistry::registerDevice(derived->getId(), this);
    }
};

#endif // CONTROLLABLE_MIXIN_H
