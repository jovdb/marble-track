/**
 * @file WebSocketManager.h
 * @brief WebSocket management and message handling for marble track system
 * 
 * This module handles WebSocket events, message processing, and client management,
 * integrating both WebSocket infrastructure and message handling logic.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class WebSocketManager {
private:
    AsyncWebSocket ws;
    
    // Static callbacks for WebSocket events
    static void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    
    // Internal message handling methods
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

public:
    WebSocketManager(const char* path = "/ws");
    void setup(AsyncWebServer& server);
    void loop();
    void notifyClients(String state);
    
    // Optional callback functions for custom event handling
    void (*onClientConnected)(AsyncWebSocketClient *client) = nullptr;
    void (*onClientDisconnected)(AsyncWebSocketClient *client) = nullptr;
};

#endif // WEBSOCKET_MANAGER_H
