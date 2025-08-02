#ifndef WEBSITE_HOST_H
#define WEBSITE_HOST_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

class WebsiteHost {
private:
    const char* ssid;
    const char* password;
    AsyncWebServer* server;
    
    // Private methods
    void initWiFi();
    void initLittleFS();
    void setupRoutes();

public:
    WebsiteHost(const char* ssid, const char* password);
    void setup(AsyncWebServer& serverRef);
};

#endif
