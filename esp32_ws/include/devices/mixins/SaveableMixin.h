/**
 * @file SaveableMixin.h
 * @brief Configuration persistence mixin
 *
 * Adds JSON config save/load capabilities to any device class.
 * The derived class must implement:
 * - void loadConfigFromJson(const JsonDocument &config)
 * - void saveConfigToJson(JsonDocument &doc) const
 *
 * Usage:
 *   class MyDevice : public DeviceBase, public SaveableMixin<MyDevice> {
 *       void loadConfigFromJson(const JsonDocument &config) override { ... }
 *       void saveConfigToJson(JsonDocument &doc) const override { ... }
 *   };
 */

#ifndef SAVEABLE_MIXIN_H
#define SAVEABLE_MIXIN_H

#include <ArduinoJson.h>
#include <Arduino.h>

/**
 * @class SaveableMixin
 * @brief Mixin that adds configuration persistence
 * @tparam Derived The derived class (CRTP pattern)
 *
 * Provides getConfig() and setConfig() that delegate to
 * the derived class's loadConfigFromJson/saveConfigToJson.
 */
template <typename Derived>
class SaveableMixin
{
public:
    virtual ~SaveableMixin() = default;

    /**
     * @brief Get the full configuration as JSON
     * @return JsonDocument with id, type, and device-specific config
     */
    JsonDocument getConfig() const
    {
        JsonDocument doc;
        const auto *derived = static_cast<const Derived *>(this);

        // Add base identity (assumes DeviceBase interface)
        doc["id"] = derived->getId();
        doc["type"] = derived->getType();
        doc["name"] = derived->getName();

        // Let derived class add its specific config
        derived->saveConfigToJson(doc);

        return doc;
    }

    /**
     * @brief Apply configuration from JSON
     * @param config JsonDocument containing configuration
     */
    void setConfig(const JsonDocument &config)
    {
        auto *derived = static_cast<Derived *>(this);

        // Extract name if present
        if (config["name"].is<String>())
        {
            derived->setName(config["name"].as<String>());
        }

        // Let derived class extract its specific config
        derived->loadConfigFromJson(config);
    }

    /**
     * @brief Setup with configuration
     * @param config Optional JSON configuration
     * @return true if setup successful
     */
    bool setupWithConfig(const JsonDocument &config = JsonDocument())
    {
        setConfig(config);
        static_cast<Derived *>(this)->setup();
        return true;
    }

protected:
    /**
     * @brief Load device-specific config from JSON
     * Must be implemented by derived class.
     * @param config The JSON document to read from
     */
    virtual void loadConfigFromJson(const JsonDocument &config) = 0;

    /**
     * @brief Save device-specific config to JSON
     * Must be implemented by derived class.
     * @param doc The JSON document to extend
     */
    virtual void saveConfigToJson(JsonDocument &doc) const = 0;
};

#endif // SAVEABLE_MIXIN_H
