#ifndef WEBSITE_HOST_H
#define WEBSITE_HOST_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include "Network.h"

class WebsiteHost {
private:
    Network* network;
    AsyncWebServer* server;
    
    // Private methods
    void initLittleFS();
    void setupRoutes();

public:
    WebsiteHost(Network* networkInstance);
    void setup(AsyncWebServer& serverRef);
};

#endif
