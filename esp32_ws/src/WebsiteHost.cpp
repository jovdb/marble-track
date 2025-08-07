#include "WebsiteHost.h"

WebsiteHost::WebsiteHost(Network* networkInstance)
    : network(networkInstance), server(nullptr)
{
}

void WebsiteHost::initLittleFS()
{
    Serial.print("Mounting file system...");
    if (!LittleFS.begin(true))
    {
        Serial.println(": ERROR mounting");
    }
    else
    {
        Serial.println(": OK");
    }
}

void WebsiteHost::setupRoutes()
{
    if (server == nullptr)
        return;

    // Web Server Root URL with debugging
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        Serial.println("Root page requested");
        if (LittleFS.exists("/index.html")) {
            Serial.println("index.html found, serving file");
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            Serial.println("index.html NOT found in LittleFS");
            
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

    // Network status endpoint (enhanced to use Network class)
    server->on("/network-status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        request->send(200, "application/json", network->getStatusJSON()); });

    // Backwards compatibility for WiFi status endpoint
    server->on("/wifi-status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        request->send(200, "application/json", network->getStatusJSON()); });

    server->serveStatic("/", LittleFS, "/");
}

void WebsiteHost::setup(AsyncWebServer &serverRef)
{
    server = &serverRef;
    initLittleFS();
    setupRoutes();
}
