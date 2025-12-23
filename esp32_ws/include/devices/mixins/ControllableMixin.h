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

/**
 * @class ControllableMixin
 * @brief Mixin that provides state serialization capability for WebSocket control
 *
 * The derived class must implement:
 * - void addStateToJson(JsonDocument &doc) - Add device state to a JSON document
 * Automatically registers itself with DeviceBase::registerMixin("ControllableMixin")
 */
template <typename Derived>
class ControllableMixin
{
public:
    /**
     * @brief Pure virtual method that derived classes must implement
     * Serializes device state to JSON for WebSocket transmission
     */
    virtual void addStateToJson(JsonDocument &doc) = 0;

protected:
    ControllableMixin()
    {
        // Register this mixin with the base class
        static_cast<Derived *>(this)->registerMixin("controllable");
    }
    virtual ~ControllableMixin() = default;
};

#endif // CONTROLLABLE_MIXIN_H
