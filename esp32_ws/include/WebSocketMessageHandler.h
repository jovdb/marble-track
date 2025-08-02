/**
 * @file WebSocketMessageHandler.h
 * @brief WebSocket message handling for marble track system
 * 
 * This module handles WebSocket events and message processing,
 * separating WebSocket logic from the main application file.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef WEBSOCKET_MESSAGE_HANDLER_H
#define WEBSOCKET_MESSAGE_HANDLER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "WebSocketManager.h"

/**
 * @brief Initialize the WebSocket message handler
 * @param wsManager Reference to the WebSocket manager instance
 */
void initializeWebSocketHandler(WebSocketManager& wsManager);

/**
 * @brief Handle incoming WebSocket messages
 * @param arg Frame info argument
 * @param data Message data
 * @param len Data length
 */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

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
                      AwsEventType type, void *arg, uint8_t *data, size_t len);

/**
 * @brief Set the WebSocket manager reference for message handling
 * @param wsManager Reference to the WebSocket manager instance
 */
void setWebSocketManager(WebSocketManager& wsManager);

#endif // WEBSOCKET_MESSAGE_HANDLER_H
