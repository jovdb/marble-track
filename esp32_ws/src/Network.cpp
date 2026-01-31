#ifdef ESP32
#include <mdns.h>
#endif
#include "Network.h"
#include "Logging.h"

Network::Network(const char *wifi_ssid, const char *wifi_password)
    : _wifi_ssid(wifi_ssid), _wifi_password(wifi_password), _currentMode(NetworkMode::DISCONNECTED), _dnsServer(nullptr),
      _isConnecting(false), _connectionStartTime(0), _wifiConnectionAttempted(false)
{
}

Network::Network(const NetworkSettings& settings)
    : _wifi_ssid(settings.ssid), _wifi_password(settings.password), _currentMode(NetworkMode::DISCONNECTED), _dnsServer(nullptr),
      _isConnecting(false), _connectionStartTime(0), _wifiConnectionAttempted(false)
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
    // Reset connection state
    _isConnecting = false;
    _connectionStartTime = 0;
    _wifiConnectionAttempted = false;
    _currentMode = NetworkMode::DISCONNECTED;

    // Start WiFi connection attempt (non-blocking)
    if (!_wifi_ssid.isEmpty())
    {
        MLOG_INFO("Starting WiFi connection to '%s' (non-blocking)...", _wifi_ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(_wifi_ssid.c_str(), _wifi_password.c_str());
        _isConnecting = true;
        _connectionStartTime = millis();
        _wifiConnectionAttempted = true;
    }
    else
    {
        MLOG_INFO("WiFi SSID not configured, will start Access Point mode");
        _wifiConnectionAttempted = true; // Skip WiFi attempt
    }

    return true; // Setup initiated successfully
}

bool Network::startAccessPoint()
{
    MLOG_INFO("Creating own network '%s' ...", AP_SSID);

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
        MLOG_INFO(": OK: http://%s", IP.toString().c_str());
        MLOG_INFO("AP mode SSID: %s, IP: %s, StationCount: %d", AP_SSID, IP.toString().c_str(), WiFi.softAPgetStationNum());
        MLOG_INFO("Captive portal enabled - all web requests will redirect here");
        return true;
    }
    else
    {
        MLOG_ERROR(": ERROR: Failed to start Access Point!");
        return false;
    }
}

void Network::setupMDNS()
{
    // Start mDNS responder for marble-track.local using ESP-IDF mDNS
    if (mdns_init() == ESP_OK)
    {
        // Set hostname without .local suffix - mDNS will append it automatically
        if (mdns_hostname_set("marble-track") == ESP_OK)
        {
            mdns_instance_name_set("Marble Track Controller");
            
            // Add HTTP service
            if (mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0) == ESP_OK)
            {
                MLOG_INFO("mDNS: http://marble-track.local : OK");
            }
            else
            {
                MLOG_WARN("mDNS: HTTP service registration failed");
            }
        }
        else
        {
            MLOG_ERROR("mDNS: hostname setup failed");
        }
    }
    else
    {
        MLOG_ERROR("mDNS: initialization failed");
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
    JsonDocument doc;

    // Mode information
    switch (_currentMode)
    {
    case NetworkMode::WIFI_CLIENT:
    {
        doc["mode"] = "client";
        doc["connected"] = true;
        doc["ssid"] = _wifi_ssid;
        doc["ip"] = WiFi.localIP().toString();
        const int rssi = WiFi.RSSI();
        doc["rssi"] = rssi;
        break;
    }

    case NetworkMode::ACCESS_POINT:
    {
        doc["mode"] = "ap";
        doc["connected"] = true;
        doc["ssid"] = AP_SSID;
        doc["ip"] = WiFi.softAPIP().toString();
        const int clients = WiFi.softAPgetStationNum();
        doc["clients"] = clients;
        break;
    }

    case NetworkMode::DISCONNECTED:
    default:
        doc["mode"] = "disconnected";
        doc["connected"] = false;
        doc["ssid"] = "";
        doc["ip"] = "0.0.0.0";
        break;
    }

    String json;
    serializeJson(doc, json);
    return json;
}void Network::processCaptivePortal()
{
    if (_currentMode == NetworkMode::ACCESS_POINT && _dnsServer != nullptr)
    {
        _dnsServer->processNextRequest();
    }
}

void Network::loop()
{
    // Handle WiFi connection process
    if (_isConnecting)
    {
        wl_status_t status = WiFi.status();
        
        if (status == WL_CONNECTED)
        {
            // WiFi connected successfully
            _isConnecting = false;
            _currentMode = NetworkMode::WIFI_CLIENT;
            MLOG_INFO("Connected to WiFi: http://%s", WiFi.localIP().toString().c_str());
            
            // Start mDNS
            setupMDNS();
        }
        else if (millis() - _connectionStartTime >= WIFI_TIMEOUT_MS)
        {
            // Connection timeout - stop trying WiFi and start AP mode
            _isConnecting = false;
            MLOG_ERROR("WiFi connection timeout - starting Access Point mode");
            WiFi.disconnect();
            
            if (startAccessPoint())
            {
                _currentMode = NetworkMode::ACCESS_POINT;
                setupMDNS();
            }
            else
            {
                _currentMode = NetworkMode::DISCONNECTED;
                MLOG_ERROR("Failed to start Access Point - no network connection!");
            }
        }
    }
    
    // Handle captive portal processing
    processCaptivePortal();
}

NetworkMode Network::applySettings(const NetworkSettings &settings)
{
    _wifi_ssid = settings.ssid;
    _wifi_password = settings.password;

    if (_dnsServer != nullptr)
    {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }

    WiFi.disconnect(true, true);
    WiFi.softAPdisconnect(true);

#ifdef ESP32
    mdns_free();
#endif

    // Reset state and restart setup
    _currentMode = NetworkMode::DISCONNECTED;
    _isConnecting = false;
    _connectionStartTime = 0;
    _wifiConnectionAttempted = false;

    setup();

    return _currentMode;
}
