#include "WebsiteHost.h"

WebsiteHost::WebsiteHost(const char* ssid, const char* password) 
    : ssid(ssid), password(password), server(nullptr) {
}

void WebsiteHost::initLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("An error has occurred while mounting LittleFS");
    } else {
        Serial.println("LittleFS mounted successfully");
    }
}

void WebsiteHost::initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void WebsiteHost::setupRoutes() {
    if (server == nullptr) return;
    
    // Web Server Root URL with debugging
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Root page requested");
        if (LittleFS.exists("/index.html")) {
            Serial.println("index.html found, serving file");
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            Serial.println("index.html NOT found in LittleFS");
            request->send(404, "text/plain", "File not found");
        }
    });

    // Debug route to list files
    server->on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        String message = "LittleFS Debug:\n";
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while(file) {
            message += "File: " + String(file.name()) + " Size: " + String(file.size()) + "\n";
            file = root.openNextFile();
        }
        request->send(200, "text/plain", message);
    });

    server->serveStatic("/", LittleFS, "/");
}

void WebsiteHost::setup(AsyncWebServer& serverRef) {
    server = &serverRef;
    initWiFi();
    initLittleFS();
    setupRoutes();
    Serial.println("Website host setup complete");
}
