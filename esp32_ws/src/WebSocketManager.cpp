#include "WebSocketManager.h"
#include "DeviceManager.h"
#include "TimeManager.h"

// Static instance for callback access (simplified to single instance)
static WebSocketManager *instance = nullptr;

String createJsonResponse(bool success, const String &message, const String &data, const String &requestId)
{
    JsonDocument response;
    response["success"] = success;
    response["message"] = message;
    response["timestamp"] = TimeManager::getCurrentTimestamp();

    if (requestId.length() > 0)
    {
        response["requestId"] = requestId;
    }

    if (data.length() > 0)
    {
        JsonDocument dataDoc;
        deserializeJson(dataDoc, data);
        response["data"] = dataDoc;
    }

    String jsonString;
    serializeJson(response, jsonString);
    return jsonString;
}

/**
 * @brief Handle incoming WebSocket messages
 */
void WebSocketManager::handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    // Check if this is a complete, single-frame text message
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT))
        return;

    data[len] = 0;
    String message = (char *)data;
    Serial.println("WebSocket: " + message);

    // Parse as JSON
    JsonDocument doc;
    if (deserializeJson(doc, message))
    {
        String errorResponse = createJsonResponse(false, "Invalid JSON format", "", "");
        notifyClients(errorResponse);
        return;
    }

    // Extract action (check both root and data field)
    String action = doc["action"] | "";
    if (action.isEmpty() && doc["data"].is<JsonObject>())
    {
        action = doc["data"]["action"] | "";
    }

    // Handle special actions
    if (action == "restart")
    {
        handleRestart();
        return;
    }

    if (action == "device-fn")
    {
        handleDeviceFunction(doc);
        return;
    }
}

// Global function wrapper for callback compatibility
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    if (instance)
    {
        instance->handleWebSocketMessage(arg, data, len);
    }
}

void WebSocketManager::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
        Serial.printf("WebSocket client #%u pong\n", client->id());
        break;
    case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u error\n", client->id());
        break;
    }
}

WebSocketManager::WebSocketManager(const char *path) : ws(path)
{
    instance = this;
}

void WebSocketManager::setup(AsyncWebServer &server)
{
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               {
        if (instance) {
            instance->onEvent(server, client, type, arg, data, len);
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

void WebSocketManager::setDeviceManager(DeviceManager *deviceManager)
{
    this->deviceManager = deviceManager;
}

void WebSocketManager::handleRestart()
{
    String response = createJsonResponse(true, "Device restart initiated", "", "");
    notifyClients(response);
    Serial.println("Restarting device in 2 seconds...");
    delay(2000);
    ESP.restart();
}

void WebSocketManager::handleDeviceFunction(JsonDocument &doc)
{
    // Extract device info from either root or data field
    String deviceId = doc["deviceId"] | "";
    String functionName = doc["fn"] | "";

    if (deviceId.isEmpty() && doc["data"].is<JsonObject>())
    {
        JsonObject dataObj = doc["data"];
        deviceId = dataObj["deviceId"] | "";
        functionName = dataObj["fn"] | "";
    }

    String response;

    if (!deviceManager)
    {
        response = createJsonResponse(false, "DeviceManager not available", "", "");
    }
    else
    {
        IControllable *controllable = deviceManager->getControllableById(deviceId);
        if (!controllable)
        {
            response = createJsonResponse(false, "Device not found or not controllable: " + deviceId, "", "");
        }
        else
        {
            Serial.println("Executing function '" + functionName + "' on device '" + deviceId + "'");
            JsonObject payload = doc.as<JsonObject>();
            bool success = controllable->control(functionName, &payload);

            response = createJsonResponse(success,
                                          success ? "Device function executed successfully" : "Device function execution failed",
                                          "", "");
        }
    }

    notifyClients(response);
}
