/**
 * @file Network.cpp
 * @brief Implementation of Network management class
 *
 * This file contains the implementation of the Network class methods
 * for handling WiFi connection and Access Point fallback functionality.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "Network.h"

// Network configuration constants
const char* Network::AP_SSID = "MarbleTrack-Setup";
const char* Network::AP_PASSWORD = "marble123";
const unsigned long Network::WIFI_TIMEOUT_MS = 20000; // 20 seconds
const unsigned long Network::CONNECTION_CHECK_INTERVAL_MS = 500; // 0.5 seconds

Network::Network(const char* wifi_ssid, const char* wifi_password)
    : _wifi_ssid(wifi_ssid), _wifi_password(wifi_password), _currentMode(NetworkMode::DISCONNECTED)
{
}

bool Network::initialize()
{
    Serial.println("=== Network Initialization ===");
    
    // First, try to connect to WiFi
    if (connectToWiFi())
    {
        _currentMode = NetworkMode::WIFI_CLIENT;
        Serial.println("Network initialized successfully in WiFi Client mode");
        printConnectionStatus();
        return true;
    }
    
    // If WiFi fails, start Access Point
    Serial.println("WiFi connection failed, starting Access Point...");
    if (startAccessPoint())
    {
        _currentMode = NetworkMode::ACCESS_POINT;
        Serial.println("Network initialized successfully in Access Point mode");
        printConnectionStatus();
        return true;
    }
    
    // If both fail
    _currentMode = NetworkMode::DISCONNECTED;
    Serial.println("ERROR: Network initialization failed!");
    return false;
}

bool Network::connectToWiFi()
{
    Serial.printf("Attempting to connect to WiFi network: %s\n", _wifi_ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifi_ssid, _wifi_password);
    
    Serial.print("Connecting");
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS)
    {
        delay(CONNECTION_CHECK_INTERVAL_MS);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println(" SUCCESS!");
        Serial.printf("Connected to: %s\n", _wifi_ssid);
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        return true;
    }
    else
    {
        Serial.println(" FAILED!");
        Serial.printf("Connection timeout after %lu seconds\n", WIFI_TIMEOUT_MS / 1000);
        WiFi.disconnect();
        return false;
    }
}

bool Network::startAccessPoint()
{
    Serial.printf("Starting Access Point: %s\n", AP_SSID);
    
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (result)
    {
        IPAddress IP = WiFi.softAPIP();
        Serial.println("Access Point started successfully!");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("IP Address: %s\n", IP.toString().c_str());
        Serial.println("Clients can connect and access the web interface");
        return true;
    }
    else
    {
        Serial.println("ERROR: Failed to start Access Point!");
        return false;
    }
}

String Network::getConnectionInfo() const
{
    switch (_currentMode)
    {
        case NetworkMode::WIFI_CLIENT:
            return "WiFi Client: " + String(_wifi_ssid) + " / IP: " + WiFi.localIP().toString();
            
        case NetworkMode::ACCESS_POINT:
            return "Access Point: " + String(AP_SSID) + " / IP: " + WiFi.softAPIP().toString();
            
        case NetworkMode::DISCONNECTED:
        default:
            return "Network: Disconnected";
    }
}

IPAddress Network::getIPAddress() const
{
    switch (_currentMode)
    {
        case NetworkMode::WIFI_CLIENT:
            return WiFi.localIP();
            
        case NetworkMode::ACCESS_POINT:
            return WiFi.softAPIP();
            
        case NetworkMode::DISCONNECTED:
        default:
            return IPAddress(0, 0, 0, 0);
    }
}

String Network::getStatusJSON() const
{
    String json = "{";
    
    // Mode information
    switch (_currentMode)
    {
        case NetworkMode::WIFI_CLIENT:
            json += "\"mode\":\"client\",";
            json += "\"connected\":true,";
            json += "\"ssid\":\"" + String(_wifi_ssid) + "\",";
            json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI());
            break;
            
        case NetworkMode::ACCESS_POINT:
            json += "\"mode\":\"ap\",";
            json += "\"connected\":true,";
            json += "\"ssid\":\"" + String(AP_SSID) + "\",";
            json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
            json += "\"clients\":" + String(WiFi.softAPgetStationNum());
            break;
            
        case NetworkMode::DISCONNECTED:
        default:
            json += "\"mode\":\"disconnected\",";
            json += "\"connected\":false,";
            json += "\"ssid\":\"\",";
            json += "\"ip\":\"0.0.0.0\"";
            break;
    }
    
    json += "}";
    return json;
}

void Network::printConnectionStatus() const
{
    Serial.println("\n=== NETWORK STATUS ===");
    Serial.println(getConnectionInfo());
    
    switch (_currentMode)
    {
        case NetworkMode::WIFI_CLIENT:
            Serial.printf("Access your device at: http://%s\n", WiFi.localIP().toString().c_str());
            break;
            
        case NetworkMode::ACCESS_POINT:
            Serial.printf("Connect to WiFi: '%s' (password: %s)\n", AP_SSID, AP_PASSWORD);
            Serial.printf("Then access: http://%s\n", WiFi.softAPIP().toString().c_str());
            break;
            
        case NetworkMode::DISCONNECTED:
            Serial.println("No network connection available");
            break;
    }
    
    Serial.println("======================\n");
}
