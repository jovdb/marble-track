/**
 * @file ConfigMixin.h
 * @brief Generic configuration management mixin
 *
 * Provides config tracking for any device.
 * The derived class can access and set config via getConfig()/setConfig().
 */

#ifndef CONFIG_MIXIN_H
#define CONFIG_MIXIN_H

/**
 * @class ConfigMixin
 * @brief Mixin for generic configuration management
 * @tparam Derived The derived class (CRTP pattern)
 * @tparam ConfigType The config struct type for this device
 */
template <typename Derived, typename ConfigType>
class ConfigMixin
{
public:
    virtual ~ConfigMixin() = default;

    /**
     * @brief Get config (read-only reference)
     * @return Reference to current config
     */
    const ConfigType& getConfig() const
    {
        return _config;
    }

    /**
     * @brief Set config
     * @param newConfig New configuration to set
     */
    void setConfig(const ConfigType& newConfig)
    {
        _config = newConfig;
    }

protected:
    /**
     * @brief Protected config member - use getConfig()/setConfig()
     */
    ConfigType _config;
};

#endif // CONFIG_MIXIN_H
