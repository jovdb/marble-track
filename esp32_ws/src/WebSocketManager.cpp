#include "WebSocketManager.h"
#include "JsonMessageHandler.h"

// Static instance pointer for callback access
WebSocketManager *WebSocketManager_instance = nullptr;
// Static reference to WebSocket manager for message handling
static WebSocketManager *wsManagerRef = nullptr;

/**
 * @brief Handle incoming WebSocket messages
 * @param arg Frame info argument
 * @param data Message data
 * @param len Data length
 */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;
        String message = (char *)data;
        Serial.println("WebSocket: Received message: " + message);

        // Parse JSON message and handle both direct action format and nested data format
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error)
        {
            Serial.println("JSON parse error: " + String(error.c_str()));
            String errorResponse = createJsonResponse(false, "Invalid JSON format: " + String(error.c_str()));
            if (wsManagerRef != nullptr)
            {
                wsManagerRef->notifyClients(errorResponse);
            }
            return;
        }

        // Check if action is at root level or in data field
        String action = doc["action"] | "";
        String deviceId = doc["deviceId"] | "";
        String functionName = doc["fn"] | "";

        // If no action at root level, check in data field
        if (action.isEmpty() && doc["data"].is<JsonObject>())
        {
            JsonObject dataObj = doc["data"];
            action = dataObj["action"] | "";
            deviceId = dataObj["deviceId"] | "";
            functionName = dataObj["fn"] | "";
        }

        if (action == "restart")
        {
            // Notify clients that restart is being initiated
            if (wsManagerRef != nullptr)
            {
                String response = createJsonResponse(true, "Device restart initiated", "restart");
                wsManagerRef->notifyClients(response);
            }

            // Handle restart action
            Serial.println("Restarting device in 2 seconds...");
            delay(2000);
            ESP.restart();
        }
        else if (action == "device-fn")
        {
            // Device function variables are already extracted above
            Serial.println("Device function called - Device: " + deviceId + ", Function: " + functionName);
            // TODO: Implement device function handling using DeviceManager
            String response = createJsonResponse(true, "Device function executed", "device-fn");
            if (wsManagerRef != nullptr)
            {
                wsManagerRef->notifyClients(response);
            }
        }
        else if (action == "get_info")
        {
            // Handle get_info action - let JsonMessageHandler process this normally
        }
        else
        {
            // For unknown actions, let JsonMessageHandler handle them
            Serial.println("Action '" + action + "' will be processed by JsonMessageHandler");
        }

        // Use the JSON message handler to process the message
        String response = handleJsonMessage(message);
        if (wsManagerRef != nullptr)
        {
            wsManagerRef->notifyClients(response);
        }
    }
}

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
        handleWebSocketMessage(arg, data, len);
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
    wsManagerRef = this; // Set up the reference for message handling
}

void WebSocketManager::setup(AsyncWebServer &server)
{
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               {
        if (WebSocketManager_instance) {
            WebSocketManager_instance->onEvent(server, client, type, arg, data, len);
        } });
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
