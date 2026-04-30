/**
 * @file Network.h
 * @brief Network management class for WiFi and Access Point functionality
 *
 * This class handles WiFi connection with automatic fallback to Access Point mode
 * when the specified WiFi network is not available.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include "Config.h"
#include "NetworkSettings.h"

enum class NetworkMode
{
    WIFI_CLIENT,
    ACCESS_POINT,
    DISCONNECTED
};

class Network
{
public:
    /**
     * @brief Get the hostname used for mDNS and OTA
     * @return String hostname
     */
    String getHostname() const;
    /**
     * @brief Constructor with individual WiFi credentials
     * @param wifi_ssid WiFi network SSID to connect to
     * @param wifi_password WiFi network password
     */
    Network(const char *wifi_ssid, const char *wifi_password);

    /**
     * @brief Constructor with NetworkSettings object
     * @param settings NetworkSettings object containing SSID and password
     */
    Network(const NetworkSettings& settings);

    /**
     * @brief Destructor - cleans up DNS server
     */
    ~Network();

    /**
     * @brief Initialize network connection (WiFi with AP fallback)
     * @return true if initialization was successful
     */
    bool setup();

    /**
     * @brief Apply new WiFi credentials, reconnecting or falling back to AP mode
     * @param settings Network settings to apply
     * @return Resulting network mode after attempting to apply the settings
     */
    NetworkMode applySettings(const NetworkSettings &settings);

    /**
     * @brief Get current network mode
     * @return Current NetworkMode (WIFI_CLIENT, ACCESS_POINT, or DISCONNECTED)
     */
    NetworkMode getCurrentMode() const { return _currentMode; }

    /**
     * @brief Get current network mode (alternative method name)
     * @return Current NetworkMode (WIFI_CLIENT, ACCESS_POINT, or DISCONNECTED)
     */
    NetworkMode getMode() const { return _currentMode; }

    /**
     * @brief Check if network mode has changed in the current loop iteration
     * @return true if mode changed in this loop, false otherwise
     *
     * @deprecated Prefer onModeChanged() instead. This polling flag is only
     * valid for one loop tick and is easy to miss; the callback fires reliably
     * on every transition.
     */
    bool isModeChanged() const { return _isModeChanged; }

    /**
     * @brief Subscribe to network mode change events.
     *
     * The callback fires once per transition (immediately, inside loop()),
     * receiving the new NetworkMode. Multiple subscribers are supported.
     * Subscribers do not need to worry about polling timing.
     */
    using ModeChangeCallback = std::function<void(NetworkMode)>;
    void onModeChanged(ModeChangeCallback cb) { _modeChangeCallbacks.push_back(std::move(cb)); }

    /**
     * @brief Check if WiFi is connected
     * @return true if connected to WiFi network, false otherwise
     */
    bool isWiFiConnected() const { return _currentMode == NetworkMode::WIFI_CLIENT; }

    /**
     * @brief Get connection information string
     * @return String with current connection details
     */
    String getConnectionInfo() const;

    /**
     * @brief Get current IP address
     * @return IPAddress object with current IP
     */
    IPAddress getIPAddress() const;

    /**
     * @brief Get network status as JSON
     * @return JSON string with network status information
     */
    String getStatusJSON() const;

    /**
     * @brief Process captive portal (call in main loop when in AP mode)
     */
    void processCaptivePortal();

    /**
     * @brief Process network operations (call in main loop)
     */
    void loop();

    /**
     * @brief Check if network is currently connecting
     * @return true if attempting to connect, false otherwise
     */
    bool isConnecting() const { return _isConnecting; }

private:
    // WiFi credentials
    String _wifi_ssid;
    String _wifi_password;

    // Current network state
    NetworkMode _currentMode;

    // DNS server for captive portal
    DNSServer *_dnsServer;

    // Connection state tracking for non-blocking operation
    bool _isConnecting;
    unsigned long _connectionStartTime;
    bool _wifiConnectionAttempted;
    bool _isModeChanged;

    // Subscribers notified on every mode transition (event-based alternative
    // to polling isModeChanged()). Fired from within loop() with the new mode.
    std::vector<ModeChangeCallback> _modeChangeCallbacks;

    // Helper: set _isModeChanged and fire all subscribers.
    void notifyModeChanged();

    // Private methods
    bool startAccessPoint();
    void setupMDNS();
};

#endif // NETWORK_H
