/**
 * @file WebSocketMessageHandler.cpp
 * @brief Implementation of WebSocket message handling
 * 
 * This file contains the implementation of WebSocket message handling
 * functions for the marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "WebSocketMessageHandler.h"
#include "JsonMessageHandler.h"

// Static reference to WebSocket manager
static WebSocketManager* wsManagerRef = nullptr;

/**
 * @brief Initialize the WebSocket message handler
 * @param wsManager Reference to the WebSocket manager instance
 */
void initializeWebSocketHandler(WebSocketManager& wsManager) {
    wsManagerRef = &wsManager;
    wsManager.onMessageReceived = handleWebSocketMessage;
    Serial.println("WebSocketMessageHandler: Initialized successfully");
}

/**
 * @brief Set the WebSocket manager reference for message handling
 * @param wsManager Reference to the WebSocket manager instance
 */
void setWebSocketManager(WebSocketManager& wsManager) {
    wsManagerRef = &wsManager;
}

/**
 * @brief Handle incoming WebSocket messages
 * @param arg Frame info argument
 * @param data Message data
 * @param len Data length
 */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char *)data;
        Serial.println("WebSocket: Received message: " + message);

        // Use the JSON message handler to process the message
        String response = handleJsonMessage(message);
        if (wsManagerRef != nullptr) {
            wsManagerRef->notifyClients(response);
        }
        
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

/**
 * @brief Handle WebSocket events (connect, disconnect, data, etc.)
 * @param server WebSocket server instance
 * @param client WebSocket client instance
 * @param type Event type
 * @param arg Additional arguments
 * @param data Event data
 * @param len Data length
 */
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket: Client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket: Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
            
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}
