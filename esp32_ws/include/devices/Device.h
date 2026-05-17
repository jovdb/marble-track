/**
 * @file Device.h
 * @brief Minimal base class for all devices using composition pattern
 *
 * This is the core device class that only provides:
 * - Identity (id, type, name)
 * - setup() and loop() lifecycle
 * - Children management
 *
 * All other functionality is added via composition mixins.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @class Device
 * @brief Minimal base class for devices
 *
 * Provides only core identity and lifecycle. Extend with mixins for:
 * - RTOS task support (withRtos)
 * - Configuration persistence (makeSerializable)
 * - WebSocket control (makeControllable)
 *
 * Two-phase initialization contract for serializable devices:
 *   Phase 1 (constructor): Build the device in a working DEFAULT state.
 *     The convention is to extract the defaults into a private
 *     applyDefaultConfig() helper called from the constructor, so the
 *     default values live in one labelled place rather than being mixed
 *     in with other construction logic. After Phase 1 the device is
 *     usable even if no /config.json exists.
 *   Phase 2 (after construction): DeviceManager calls jsonToConfig() with
 *     the persisted JSON, overriding the Phase-1 defaults field-by-field.
 *     Missing JSON fields keep their default values.
 *
 * Note: applyDefaultConfig() is intentionally NOT declared as a virtual on
 * the base class because virtual dispatch from a base-class constructor
 * would not reach the derived override. Each derived device is responsible
 * for calling its own applyDefaultConfig() from its constructor.
 */
class Device
{
public:
    Device(const String &id, const String &type);
    virtual ~Device() = default;

    // Lifecycle
    // Default setup calls setup on children
    virtual void setup();
    virtual void teardown();
    virtual void loop();

    // Check if setup has been called
    bool isSetup() const { return _isInitialized; }

    // Identity
    String getId() const { return _id; }
    String getType() const { return _type; }
    String getName() const { return _name; }
    void setName(const String &name) { _name = name; }
    virtual String toString() const;

    // Hierarchy
    void addChild(Device *child);
    const std::vector<Device *> &getChildren() const { return _children; }
    Device *getChildById(const String &id) const;

    template <typename T>
    T *getChildByIdAs(const String &id) const
    {
        return static_cast<T *>(getChildById(id));
    }

    // Error state (available on every device, serialized automatically by ControllableMixin)
    bool hasError() const { return !_errorCode.isEmpty(); }
    const String &getErrorCode() const { return _errorCode; }
    const String &getErrorMessage() const { return _errorMessage; }

    // Pins (for collision detection)
    virtual std::vector<String> getPins() const { return {}; }

    // Mixin detection
    bool hasMixin(const String &mixinName) const;
    const std::vector<String> &getMixins() const { return _mixins; }
    void registerMixin(const String &mixinName);

protected:
    String _id;
    String _type;
    String _name;
    bool _isInitialized = false;
    std::vector<Device *> _children;
    std::vector<String> _mixins;

    void setError(const String &errorCode, const String &errorMessage);
    void clearError();

    // Walks the subtree of _children depth-first.
    // Returns the first descendant that has an error, or nullptr.
    Device *getFirstChildWithError() const;
private:
    String _errorCode;    // Empty string = no error
    String _errorMessage;
};

#endif // DEVICE_H
