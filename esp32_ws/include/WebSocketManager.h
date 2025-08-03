#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Forward declaration
class DeviceManager;

class WebSocketManager
{
private:
    AsyncWebSocket ws;

public:
    WebSocketManager(const char *path = "/ws");
    void setup(AsyncWebServer &server);
    void loop();
    void notifyClients(String state);
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void setDeviceManager(DeviceManager* deviceManager);
};

#endif
