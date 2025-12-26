/**
 * @file SerializableMixin.h
 * @brief Mixin for devices that can save/load configuration to/from JSON
 *
 * Requires the derived class to implement:
 * - void jsonToConfig(const JsonDocument &config)
 * - void configToJson(JsonDocument &doc)
 */

#ifndef SERIALIZABLE_MIXIN_H
#define SERIALIZABLE_MIXIN_H

#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>

/**
 * @class ISerializable
 * @brief Interface for devices that support config serialization
 *
 * This interface provides a type-safe way to call serialization methods
 * on devices without knowing their concrete type.
 */
class ISerializable
{
public:
    virtual ~ISerializable() = default;

    /**
     * @brief Load device-specific config from JSON
     * @param config The JSON document to read from
     */
    virtual void jsonToConfig(const JsonDocument &config) = 0;

    /**
     * @brief Save device-specific config to JSON
     * @param doc The JSON document to extend
     */
    virtual void configToJson(JsonDocument &doc) = 0;
};

/**
 * @namespace mixins
 * @brief Registry for looking up ISerializable interfaces by device ID
 */
namespace mixins
{
    class SerializableRegistry
    {
    public:
        static void registerDevice(const String &id, ISerializable *ptr);
        static void unregisterDevice(const String &id);
        static ISerializable *get(const String &id);
    };
}

/**
 * @class SerializableMixin
 * @brief Mixin that provides config persistence capability
 *
 * The derived class must implement:
 * - void jsonToConfig(const JsonDocument &config) - Load device config from JSON
 * - void configToJson(JsonDocument &doc) - Save device config to JSON
 * Automatically registers itself with DeviceBase::registerMixin("serializable")
 * and with SerializableRegistry for lookup by device ID.
 */
template <typename Derived>
class SerializableMixin : public ISerializable
{
public:
    virtual ~SerializableMixin()
    {
        // Unregister on destruction
        auto *derived = static_cast<Derived *>(this);
        mixins::SerializableRegistry::unregisterDevice(derived->getId());
    }

protected:
    SerializableMixin()
    {
        // Register this mixin with the base class
        auto *derived = static_cast<Derived *>(this);
        derived->registerMixin("serializable");
        mixins::SerializableRegistry::registerDevice(derived->getId(), this);
    }

    /**
     * @brief Load device-specific config from JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param config The JSON document to read from
     */
    void jsonToConfig(const JsonDocument &config) override
    {
        // Default: do nothing
        (void)config; // Suppress unused parameter warning
    }

    /**
     * @brief Save device-specific config to JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param doc The JSON document to extend
     */
    void configToJson(JsonDocument &doc) override
    {
        // Default: do nothing
        (void)doc; // Suppress unused parameter warning
    }
};

#endif // SERIALIZABLE_MIXIN_H
