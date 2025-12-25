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

/**
 * @class SerializableMixin
 * @brief Mixin that provides config persistence capability
 *
 * The derived class must implement:
 * - void jsonToConfig(const JsonDocument &config) - Load device config from JSON
 * - void configToJson(JsonDocument &doc) - Save device config to JSON
 * Automatically registers itself with DeviceBase::registerMixin("serializable")
 */
template <typename Derived>
class SerializableMixin
{
protected:
    SerializableMixin()
    {
        // Register this mixin with the base class
        auto *derived = static_cast<Derived *>(this);
        derived->registerMixin("serializable");
    }

    /**
     * @brief Load device-specific config from JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param config The JSON document to read from
     */
    virtual void jsonToConfig(const JsonDocument &config)
    {
        // Default: do nothing
        (void)config; // Suppress unused parameter warning
    }

    /**
     * @brief Save device-specific config to JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param doc The JSON document to extend
     */
    virtual void configToJson(JsonDocument &doc)
    {
        // Default: do nothing
        (void)doc; // Suppress unused parameter warning
    }

public:
    virtual ~SerializableMixin() = default;
};

#endif // SAVEABLE_MIXIN_H
