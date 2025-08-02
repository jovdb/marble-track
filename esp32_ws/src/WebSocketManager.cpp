#include "WebSocketManager.h"

// Static instance pointer for callback access
WebSocketManager* WebSocketManager_instance = nullptr;

WebSocketManager::WebSocketManager(const char* path) : ws(path) {
    WebSocketManager_instance = this;
}

void WebSocketManager::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (WebSocketManager_instance == nullptr) return;
    
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            if (WebSocketManager_instance->onClientConnected) {
                WebSocketManager_instance->onClientConnected(client);
            }
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            if (WebSocketManager_instance->onClientDisconnected) {
                WebSocketManager_instance->onClientDisconnected(client);
            }
            break;
        case WS_EVT_DATA:
            if (WebSocketManager_instance->onMessageReceived) {
                WebSocketManager_instance->onMessageReceived(arg, data, len);
            }
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebSocketManager::setup(AsyncWebServer& server) {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    Serial.println("WebSocket manager setup complete");
}

void WebSocketManager::loop() {
    ws.cleanupClients();
}

void WebSocketManager::notifyClients(String state) {
    ws.textAll(state);
}
