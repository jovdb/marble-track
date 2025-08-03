/**
 * @file WebSocketManager.cpp
 * @brief Implementation of WebSocket management and message handling
 * 
 * This file contains the implementation of WebSocket infrastructure
 * and message handling for the marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "WebSocketManager.h"
#include "JsonMessageHandler.h"

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
            WebSocketManager_instance->handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebSocketManager::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char *)data;
        Serial.println("WebSocket: Received message: " + message);

        // Use the JSON message handler to process the message
        String response = handleJsonMessage(message);
        notifyClients(response);
        
        // Handle special commands that need additional processing
        JsonDocument doc;
        if (deserializeJson(doc, message) == DeserializationError::Ok) {
            String action = doc["action"] | "";
            if (action == "restart") {
                Serial.println("Restarting device in 2 seconds...");
                delay(2000);
                ESP.restart();
            }
        }
    }
}

void WebSocketManager::setup(AsyncWebServer& server) {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    Serial.println("WebSocket manager: OK");
    Serial.println("WebSocketManager: Message handling initialized");
}

void WebSocketManager::loop() {
    ws.cleanupClients();
}

void WebSocketManager::notifyClients(String state) {
    ws.textAll(state);
}
