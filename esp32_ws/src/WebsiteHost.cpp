#include "Logging.h"
#include "WebsiteHost.h"

WebsiteHost::WebsiteHost(Network *networkInstance)
    : network(networkInstance), server(nullptr)
{
}

void WebsiteHost::setupRoutes()
{
    if (server == nullptr)
        return;

    // Captive Portal Detection and Windows/Proxy Probe Short-circuit
    // These routes handle the automatic connectivity checks that devices make
    server->on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Android captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("iOS captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/connectivity-check.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Generic captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    // Windows/Proxy probe files: short-circuit to avoid LittleFS and watchdog resets
    server->on("/connecttest.txt", HTTP_ANY, [](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows connecttest.txt probe - short-circuit 200 OK");
        request->send(200, "text/plain", "OK"); });
    server->on("/wpad.dat", HTTP_ANY, [](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows wpad.dat probe - short-circuit 200 OK");
        request->send(200, "text/plain", "OK"); });

    // Web Server Root URL with debugging
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Website accessed");
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            MLOG_WARN("index.html NOT found in LittleFS. Did you upload the website?");
            
            // Create a simple fallback page with network status
            String html = "<!DOCTYPE html><html><head><title>Marble Track Control</title></head><body>";
            html += "<h1>Marble Track Control System</h1>";
            html += "<p>Web interface files not found in flash memory.</p>";
            html += "<p>Please upload the website files using PlatformIO 'Upload Filesystem Image'.</p>";
            html += "<h2>Connection Status:</h2>";
            html += "<p>" + network->getConnectionInfo() + "</p>";
            
            html += "<h2>Available files in LittleFS:</h2><ul>";
            
            File root = LittleFS.open("/");
            File file = root.openNextFile();
            while (file) {
                html += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
                file = root.openNextFile();
            }
            html += "</ul>";
            html += "<p><a href='/debug'>Debug Info</a> | <a href='/network-status'>Network Status</a></p>";
            html += "</body></html>";
            request->send(200, "text/html", html);
        } });

    // Debug route to list files
    server->on("/debug", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String message = "LittleFS Debug:\n";
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while(file) {
            message += "File: " + String(file.name()) + " Size: " + String(file.size()) + "\n";
            file = root.openNextFile();
        }
        request->send(200, "text/plain", message); });

    // Test WebSocket connectivity endpoint
    server->on("/test-ws", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        String html = "<!DOCTYPE html><html><head><title>WebSocket Test</title></head><body>";
        html += "<h1>WebSocket Connection Test</h1>";
        html += "<div id='status'>Connecting...</div>";
        html += "<div id='messages'></div>";
        html += "<script>";
        html += "const ws = new WebSocket('ws://" + network->getIPAddress().toString() + "/ws');";
        html += "const status = document.getElementById('status');";
        html += "const messages = document.getElementById('messages');";
        html += "ws.onopen = () => { status.textContent = 'Connected!'; status.style.color = 'green'; };";
        html += "ws.onclose = () => { status.textContent = 'Disconnected'; status.style.color = 'red'; };";
        html += "ws.onerror = (e) => { status.textContent = 'Error: ' + e; status.style.color = 'red'; };";
        html += "ws.onmessage = (e) => { messages.innerHTML += '<div>Received: ' + e.data + '</div>'; };";
        html += "</script></body></html>";
        request->send(200, "text/html", html); });

    // Catch-all handler for any other requests (prevents LittleFS errors)
    server->onNotFound([this](AsyncWebServerRequest *request)
                       {
        String url = request->url();
    MLOG_WARN("404 - File not found: %s - redirecting to root", url.c_str());
        
        // If it's a captive portal check we missed, redirect to root
        if (url.indexOf("204") != -1 || 
            url.indexOf("generate") != -1 ||
            url.indexOf("hotspot") != -1 ||
            url.indexOf("captive") != -1 ||
            url.indexOf("connectivity") != -1) {
            request->redirect("http://" + network->getIPAddress().toString() + "/");
        } else {
            // For other missing files, return a simple redirect to root
            request->redirect("http://" + network->getIPAddress().toString() + "/");
        } });

    server->serveStatic("/", LittleFS, "/");
}

void WebsiteHost::setup(AsyncWebServer &serverRef)
{
    server = &serverRef;
    setupRoutes();
}
