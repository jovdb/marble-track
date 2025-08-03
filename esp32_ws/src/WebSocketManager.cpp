#include "WebSocketManager.h"
#include "WebSocketMessageHandler.h"

// Static instance pointer for callback access
WebSocketManager *WebSocketManager_instance = nullptr;


void WebSocketManager::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (WebSocketManager_instance == nullptr)
        return;

    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        Serial.printf("WebSocket client #%u data received: %s\n", client->id(), data);
        onWebSocketEvent(server, client, type, arg, data, len);
        break;
    case WS_EVT_PONG:
        Serial.printf("WebSocket client #%u pong received\n", client->id());
    case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u error occurred\n", client->id());
        break;
    }
}

WebSocketManager::WebSocketManager(const char *path) : ws(path)
{
    WebSocketManager_instance = this;
}

void WebSocketManager::setup(AsyncWebServer &server)
{
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (WebSocketManager_instance) {
            WebSocketManager_instance->onEvent(server, client, type, arg, data, len);
        }
    });
    server.addHandler(&ws);
    Serial.println("WebSocket manager: OK");
}

void WebSocketManager::loop()
{
    ws.cleanupClients();
}

void WebSocketManager::notifyClients(String state)
{
    ws.textAll(state);
}
