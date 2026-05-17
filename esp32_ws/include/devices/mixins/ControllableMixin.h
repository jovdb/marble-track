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
 * - void addDeviceStateToJson(JsonDocument &doc) - Add device-specific state fields
 *
 * Error fields (errorCode / errorMessage) are injected automatically by this mixin
 * into every addStateToJson call so no call-site needs to remember to add them.
 *
 * Automatically registers itself with Device::registerMixin("ControllableMixin")
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

    /**
     * @brief Implements IControllable::addStateToJson as a final wrapper.
     * Calls addDeviceStateToJson() for device-specific fields, then always
     * appends errorCode and errorMessage from the Device base class.
     * This guarantees every caller — push notifications and pull requests alike —
     * gets the error fields without any extra code at the call site.
     */
    void addStateToJson(JsonDocument &doc) override final
    {
        auto *derived = static_cast<Derived *>(this);
        addDeviceStateToJson(doc);
        doc["errorCode"] = derived->getErrorCode();
        doc["errorMessage"] = derived->getErrorMessage();
    }

    // Provide Device virtual override via mixin when combined
    virtual IControllable *getControllableInterface() { return this; }

protected:
    /**
     * @brief Pure virtual: derived devices fill in their own state fields.
     * Do NOT add errorCode/errorMessage here; the wrapper above handles them.
     */
    virtual void addDeviceStateToJson(JsonDocument &doc) = 0;

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
     * @brief When state changes, automatically notify clients via WebSocket
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
        addStateToJson(stateDoc); // error fields are injected inside addStateToJson

        doc["state"] = stateDoc;

        String message;
        serializeJson(doc, message);
        callback(message);
    }
};

#endif // CONTROLLABLE_MIXIN_H
