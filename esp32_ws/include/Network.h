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

enum class NetworkMode {
    WIFI_CLIENT,
    ACCESS_POINT,
    DISCONNECTED
};

class Network {
public:
    /**
     * @brief Constructor
     * @param wifi_ssid WiFi network SSID to connect to
     * @param wifi_password WiFi network password
     */
    Network(const char* wifi_ssid, const char* wifi_password);
    
    /**
     * @brief Initialize network connection (WiFi with AP fallback)
     * @return true if initialization was successful
     */
    bool initialize();
    
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
     * @brief Check if device is in Access Point mode
     * @return true if in AP mode, false otherwise
     */
    bool isAccessPointMode() const { return _currentMode == NetworkMode::ACCESS_POINT; }
    
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
     * @brief Print connection status to Serial
     */
    void printConnectionStatus() const;

private:
    // WiFi credentials
    const char* _wifi_ssid;
    const char* _wifi_password;
    
    // Current network state
    NetworkMode _currentMode;
    
    // Configuration constants
    static const char* AP_SSID;
    static const char* AP_PASSWORD;
    static const unsigned long WIFI_TIMEOUT_MS;
    static const unsigned long CONNECTION_CHECK_INTERVAL_MS;
    
    // Private methods
    bool connectToWiFi();
    bool startAccessPoint();
};

#endif // NETWORK_H
