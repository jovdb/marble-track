#ifndef WEBSITE_HOST_H
#define WEBSITE_HOST_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFSManager.h"
#include "Network.h"

class WebsiteHost {
private:
    Network* network;
    AsyncWebServer* server;
    LittleFSManager littleFSManager;
    // Private methods
    void setupRoutes();

public:
    WebsiteHost(Network* networkInstance);
    void setup(AsyncWebServer& serverRef);
    void loop() { littleFSManager.loop(); }
};

#endif
