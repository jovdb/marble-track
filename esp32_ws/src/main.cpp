/**
 * @file main.cpp
 * @brief Main application file for Marble Track Controller ESP32
 * 
 * This file contains the main application logic, WebSocket event handling,
 * and system initialization for the ESP32-based marble track control system.
 * JSON message processing and hardware control are handled by separate modules.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"
#include "JsonMessageHandler.h"
#include "HardwareController.h"
#include "Led.h"

// Replace with your network credentials
// const char *ssid = "REPLACE_WITH_YOUR_SSID";
// const char *password = "REPLACE_WITH_YOUR_PASSWORD";
const char *ssid = "telenet-182FE";
const char *password = "cPQdRWmFx1eM";

// Create server and instances
AsyncWebServer server(80);
WebsiteHost websiteHost(ssid, password);
WebSocketManager wsManager("/ws");
Led ledController(2, "status_led", "Status LED");  // Pin 2, ID "status_led", Name "Status LED"

// WebSocket event handlers
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String message = (char *)data;
    Serial.println("WebSocket: Received message: " + message);

    // Use the JSON message handler to process the message
    String response = handleJsonMessage(message);
    wsManager.notifyClients(response);
    
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

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket: Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    updateConnectedClients(server->count());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket: Client #%u disconnected\n", client->id());
    updateConnectedClients(server->count());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Starting Marble Track JSON Communication System");

  // Initialize hardware components
  initializeHardware();
  
  // Initialize LED controller
  ledController.setup();
  
  // Initialize JSON message handler
  initializeJsonHandler();

  // Initialize TimeManager
  TimeManager::initialize();

  // Initialize WebsiteHost
  websiteHost.setup(server);

  // Setup WebSocket
  wsManager.onMessageReceived = handleWebSocketMessage;
  wsManager.setup(server);

  // Start server
  server.begin();
  Serial.println("Marble Track JSON Communication System initialized");
  Serial.println("JSON Commands available:");
  Serial.println("- Discovery: {\"type\": \"discovery\"}");
  Serial.println("- Control: {\"action\": \"set_direction\", \"value\": \"CW\"}");
  Serial.println("- GPIO: {\"action\": \"set_gpio\", \"pin\": 2, \"state\": true}");
  Serial.println("- Info: {\"action\": \"get_info\"}");
  Serial.println("Connect via WebSocket to receive full command examples");
  
  // Print current hardware status
  Serial.println(getHardwareStatus());
}

void loop()
{
  // Keep the WebSocket alive
  wsManager.loop();
  
  // Run LED controller loop
  ledController.loop();
  
  // Optional: Send periodic status updates (simplified)
  static unsigned long lastStatusUpdate = 0;
  const unsigned long statusUpdateInterval = 30000; // 30 seconds
  
  if (millis() - lastStatusUpdate > statusUpdateInterval) {
    // Only send status if we have some indication of connected clients
    // Note: We'll need to track this ourselves since WebSocketManager doesn't expose count
    String status = createDeviceStatusJson();
    wsManager.notifyClients(status);
    Serial.println("Sent periodic status update");
    lastStatusUpdate = millis();
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}
