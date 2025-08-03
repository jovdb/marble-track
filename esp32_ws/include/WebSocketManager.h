#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class WebSocketManager {
private:
    AsyncWebSocket ws;
    
    // Static callbacks for WebSocket events
    static void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

public:
    WebSocketManager(const char* path = "/ws");
    void setup(AsyncWebServer& server);
    void loop();
    void notifyClients(String state);
    
    // Function pointers for handling events (to be set from main.cpp)
    void (*onMessageReceived)(void *arg, uint8_t *data, size_t len) = nullptr;
    void (*onClientConnected)(AsyncWebSocketClient *client) = nullptr;
    void (*onClientDisconnected)(AsyncWebSocketClient *client) = nullptr;
};

#endif
