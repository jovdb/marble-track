#ifdef ESP32
#include <mdns.h>
#endif
#include "Network.h"
static const char *TAG = "Network";

Network::Network(const char *wifi_ssid, const char *wifi_password)
    : _wifi_ssid(wifi_ssid), _wifi_password(wifi_password), _currentMode(NetworkMode::DISCONNECTED), _dnsServer(nullptr)
{
}

String Network::getHostname() const
{
    // If you want to make this configurable, use a member variable
    // For now, return the default used in mDNS and OTA
    String hostname = "marble-track";
    return hostname;
}

static const char *AP_SSID = "MarbleTrackAP";
static const char *AP_PASSWORD = "";                           // Default password for the Access Point
static const unsigned long WIFI_TIMEOUT_MS = 10000;            // Timeout for WiFi connection
static const unsigned long CONNECTION_CHECK_INTERVAL_MS = 500; // Interval to check connection status

Network::~Network()
{
    if (_dnsServer != nullptr)
    {
        delete _dnsServer;
        _dnsServer = nullptr;
    }
}

bool Network::setup()
{
    // First, try to connect to WiFi
    if (connectToWiFi())
    {
        _currentMode = NetworkMode::WIFI_CLIENT;
    }

    // If WiFi fails, start Access Point
    if (_currentMode == NetworkMode::DISCONNECTED && startAccessPoint())
    {
        _currentMode = NetworkMode::ACCESS_POINT;
    }

    // Start mDNS responder for marble-track.local using ESP-IDF mDNS
    if (_currentMode != NetworkMode::DISCONNECTED)
    {
        // Start mDNS
        if (mdns_init() == ESP_OK)
        {
            // Set hostname without .local suffix - mDNS will append it automatically
            if (mdns_hostname_set("marble-track") == ESP_OK)
            {
                mdns_instance_name_set("Marble Track Controller");
                
                // Add HTTP service
                if (mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0) == ESP_OK)
                {
                    ESP_LOGI(TAG, "mDNS: http://marble-track.local : OK");
                }
                else
                {
                    ESP_LOGW(TAG, "mDNS: HTTP service registration failed");
                }
            }
            else
            {
                ESP_LOGE(TAG, "mDNS: hostname setup failed");
            }
        }
        else
        {
            ESP_LOGE(TAG, "mDNS: initialization failed");
        }

        return true; // Allow successful setup if mDNS fails
    }

    _currentMode = NetworkMode::DISCONNECTED;
    ESP_LOGE(TAG, "No network connection!");
    return false;
}

bool Network::connectToWiFi()
{
    ESP_LOGI(TAG, "Connect to WiFi network '%s' ..", _wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifi_ssid, _wifi_password);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS)
    {
        delay(CONNECTION_CHECK_INTERVAL_MS);
        // ESP_LOGI(TAG, "."); // Optionally log progress
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGI(TAG, ": OK, http://%s", WiFi.localIP().toString().c_str());
        // ESP_LOGI(TAG, "Signal Strength: %d dBm", WiFi.RSSI());
        return true;
    }
    else
    {
        ESP_LOGE(TAG, ": ERROR: Could not connect");
        WiFi.disconnect();
        return false;
    }
}

bool Network::startAccessPoint()
{
    ESP_LOGI(TAG, "Creating own network '%s' ...", AP_SSID);

    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(AP_SSID, AP_PASSWORD);

    if (result)
    {
        IPAddress IP = WiFi.softAPIP();

        // Set up captive portal DNS server
        if (_dnsServer == nullptr)
        {
            _dnsServer = new DNSServer();
        }

        // Start DNS server on port 53 and redirect all domains to our IP
        _dnsServer->start(53, "*", IP);

        // ESP_LOGI(TAG, "Password: %s", AP_PASSWORD);
        ESP_LOGI(TAG, ": OK: http://%s", IP.toString().c_str());
        ESP_LOGI(TAG, "AP mode SSID: %s, IP: %s, StationCount: %d", AP_SSID, IP.toString().c_str(), WiFi.softAPgetStationNum());
        ESP_LOGI(TAG, "Captive portal enabled - all web requests will redirect here");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, ": ERROR: Failed to start Access Point!");
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

void Network::processCaptivePortal()
{
    if (_currentMode == NetworkMode::ACCESS_POINT && _dnsServer != nullptr)
    {
        _dnsServer->processNextRequest();
    }
}
