/**
 * @file SaveableMixin.h
 * @brief Mixin for devices that can save/load configuration to/from JSON
 *
 * Requires the derived class to implement:
 * - void loadConfigFromJson(const JsonDocument &config)
 * - void saveConfigToJson(JsonDocument &doc)
 */

#ifndef SAVEABLE_MIXIN_H
#define SAVEABLE_MIXIN_H

#include <ArduinoJson.h>

/**
 * @class SaveableMixin
 * @brief Mixin that provides config persistence capability
 *
 * The derived class must implement:
 * - void loadConfigFromJson(const JsonDocument &config) - Load device config from JSON
 * - void saveConfigToJson(JsonDocument &doc) - Save device config to JSON
 * Automatically registers itself with DeviceBase::registerMixin("saveable")
 */
template <typename Derived>
class SaveableMixin
{
protected:
    SaveableMixin()
    {
        // Register this mixin with the base class
        auto *derived = static_cast<Derived *>(this);
        derived->registerMixin("saveable");
    }

    /**
     * @brief Load device-specific config from JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param config The JSON document to read from
     */
    virtual void loadConfigFromJson(const JsonDocument &config)
    {
        // Default: do nothing
        (void)config; // Suppress unused parameter warning
    }

    /**
     * @brief Save device-specific config to JSON
     * Default implementation does nothing. Override in derived class if needed.
     * @param doc The JSON document to extend
     */
    virtual void saveConfigToJson(JsonDocument &doc)
    {
        // Default: do nothing
        (void)doc; // Suppress unused parameter warning
    }

public:
    virtual ~SaveableMixin() = default;
};

#endif // SAVEABLE_MIXIN_H
