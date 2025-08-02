#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"

// Replace with your network credentials
// const char *ssid = "REPLACE_WITH_YOUR_SSID";
// const char *password = "REPLACE_WITH_YOUR_PASSWORD";
const char *ssid = "telenet-182FE";
const char *password = "cPQdRWmFx1eM";

// Create server and instances
AsyncWebServer server(80);
WebsiteHost websiteHost(ssid, password);
WebSocketManager wsManager("/ws");

// Variables for request handling
String message = "";
String direction = "STOP";
bool newRequest = false;

// WebSocket event handlers
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;
    direction = message.substring(message.indexOf("&") + 1, message.length());
    Serial.print("direction: ");
    Serial.println(direction);
    wsManager.notifyClients(direction);
    newRequest = true;
  }
}

void handleClientConnected(AsyncWebSocketClient *client)
{
  // Notify client of motor current state when it first connects
  wsManager.notifyClients("WELCOME");
}

void handleClientDisconnected(AsyncWebSocketClient *client)
{
  // Handle client disconnection if needed
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize website hosting with server reference
  websiteHost.setup(server);

  // Initialize time synchronization
  TimeManager::initialize();

  // Setup WebSocket manager with server reference and assign event handlers
  wsManager.onMessageReceived = handleWebSocketMessage;
  wsManager.onClientConnected = handleClientConnected;
  wsManager.onClientDisconnected = handleClientDisconnected;
  wsManager.setup(server);

  // Start the server
  server.begin();
  Serial.println("Server started on port 80");

  pinMode(1, OUTPUT);
}

void loop()
{
  // Handle WebSocket operations
  wsManager.loop();

  // Handle hardware control
  if (newRequest)
  {
    if (direction == "CW")
    {
      digitalWrite(1, HIGH);
      Serial.println("Off");
    }
    else
    {
      digitalWrite(1, LOW);
      Serial.println("On");
    }
    newRequest = false;
    
    // Send response with JavaScript-compatible timestamp (milliseconds since Unix epoch)
    unsigned long long jsTimestamp = TimeManager::getCurrentTimestamp();
    wsManager.notifyClients("{ \"type\": \"pong\", \"timestamp\": " + String(jsTimestamp) + " }");
  }
}