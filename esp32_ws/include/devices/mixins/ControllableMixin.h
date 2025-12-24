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
#include <functional>
#include "Logging.h"

using NotifyClients = std::function<void(const String &)>;

/**
 * @class ControllableMixinBase
 * @brief Non-template base to hold SHARED static callback (avoids template specialization issue)
 */
class ControllableMixinBase
{
protected:
    inline static NotifyClients s_globalNotifyClients;

public:
    /**
     * @brief Set the global notifyClients callback for ALL controllable devices
     * This uses a non-template static so all specializations share the same callback
     */
    static void setNotifyClients(NotifyClients callback)
    {
        s_globalNotifyClients = callback;
    }

    static NotifyClients getNotifyClients()
    {
        return s_globalNotifyClients;
    }
};

/**
 * @class ControllableMixin
 * @brief Mixin that provides state serialization capability for WebSocket control
 *
 * The derived class must implement:
 * - void addStateToJson(JsonDocument &doc) - Add device state to a JSON document
 * Automatically registers itself with DeviceBase::registerMixin("ControllableMixin")
 */
template <typename Derived>
class ControllableMixin : public IControllable, public ControllableMixinBase
{
public:
    virtual ~ControllableMixin()
    {
        // Unregister on destruction
        auto *derived = static_cast<Derived *>(this);
        mixins::ControllableRegistry::unregisterDevice(derived->getId());
    }

    /**
     * @brief Handle control commands for this device
     * Derived classes must implement to handle device-specific actions
     */
    virtual bool control(const String &action, JsonObject *args = nullptr) = 0;

    // Provide DeviceBase virtual override via mixin when combined
    virtual IControllable *getControllableInterface() { return this; }

protected:
    ControllableMixin()
    {
        // Register this mixin with the base class
        auto *derived = static_cast<Derived *>(this);
        derived->registerMixin("controllable");
        mixins::ControllableRegistry::registerDevice(derived->getId(), this);

        // Subscribe to state changes if the device has StateMixin
        if (derived->hasMixin("state"))
        {
            subscribeToStateChanges();
        }
    }

private:
    /**
     * @brief Subscribe to state changes from StateMixin
     */
    void subscribeToStateChanges()
    {
        auto *derived = static_cast<Derived *>(this);
        derived->onStateChange([this](void *state)
                               { this->handleStateChange(); });
    }

    /**
     * @brief Handle state change by sending WebSocket message
     */
    void handleStateChange()
    {
        NotifyClients callback = getNotifyClients();
        if (!callback)
            return;

        auto *derived = static_cast<Derived *>(this);
        JsonDocument doc;
        doc["type"] = "device-state";
        doc["deviceId"] = derived->getId();
        doc["success"] = true;

        JsonDocument stateDoc;
        addStateToJson(stateDoc);
        doc["state"] = stateDoc;

        String message;
        serializeJson(doc, message);
        callback(message);
    }
};

#endif // CONTROLLABLE_MIXIN_H
