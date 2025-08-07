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
    Serial.println("WebSocket received: " + message);

    // Parse as JSON
    JsonDocument doc;
    if (deserializeJson(doc, message))
    {
        String errorResponse = createJsonResponse(false, "Invalid JSON format", "", "");
        notifyClients(errorResponse);
        return;
    }

    // Extract type (check both root and data field)
    String type = doc["type"] | "";
    if (type.isEmpty() && doc["data"].is<JsonObject>())
    {
        type = doc["data"]["type"] | "";
    }

    // Handle special type
    if (type == "restart")
    {
        handleRestart();
        return;
    }

    if (type == "device-fn")
    {
        handleDeviceFunction(doc);
        return;
    }

    if (type == "device-get-state")
    {
        handleDeviceGetState(doc);
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
        {
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            
            // Send welcome message with connection info
            String welcome = "{\"type\":\"connection\",\"message\":\"WebSocket connected\",\"clientId\":" + String(client->id()) + "}";
            client->text(welcome);
            break;
        }
        
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
        
    case WS_EVT_DATA:
        Serial.printf("WebSocket client #%u data received\n", client->id());
        handleWebSocketMessage(arg, data, len);
        break;
        
    case WS_EVT_PONG:
        Serial.printf("WebSocket client #%u pong\n", client->id());
        break;
        
    case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u ERROR occurred\n", client->id());
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
    Serial.println("WebSocket path: /ws");
    Serial.printf("WebSocket server ready to accept connections\n");
}

void WebSocketManager::loop()
{
    ws.cleanupClients();
}

String WebSocketManager::getStatus() const
{
    return "{\"connectedClients\":" + String(ws.count()) + ",\"path\":\"/ws\"}";
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
        Device *device = deviceManager->getDeviceById(deviceId);
        if (!device)
        {
            response = createJsonResponse(false, "Device not found or not controllable: " + deviceId, "", "");
        }
        else
        {
            Serial.printf("Executing: %s[%s]\n", deviceId, functionName);
            JsonObject payload = doc.as<JsonObject>();
            bool success = device->control(functionName, &payload);

            /*
            response = createJsonResponse(success,
                                          success ? "Device function executed successfully" : "Device function execution failed",
                                          "", "");
            */
        }
    }

    if (response != nullptr)
        notifyClients(response);
}

void WebSocketManager::handleDeviceGetState(JsonDocument &doc)
{
    // Extract device info from either root or data field
    String deviceId = doc["deviceId"] | "";

    String response;

    if (!deviceManager)
    {
        broadcastState(deviceId, "", "DeviceManager not available");
    }
    else
    {
        Device *device = deviceManager->getDeviceById(deviceId);
        if (!device)
        {
            broadcastState(deviceId, "", "Device '" + deviceId + "' not found.");
        }
        else
        {
            String state = device->getState();
            broadcastState(deviceId, state, "");
        }
    }

    notifyClients(response);
}

void WebSocketManager::broadcastState(const String &deviceId, const String &stateJson, const String &error)
{
    JsonDocument doc;
    doc["type"] = "device-state";
    doc["deviceId"] = deviceId;

    // Parse the state JSON string and add it to the document
    JsonDocument stateDoc;
    if (deserializeJson(stateDoc, stateJson) == DeserializationError::Ok)
    {
        doc["state"] = stateDoc.as<JsonObject>();
    }
    else
    {
        // Fallback: add the string directly if parsing fails
        doc["state"] = stateJson;
    }
    if (!error.isEmpty())
        doc["error"] = error;

    String message;
    serializeJson(doc, message);
    printf("Websocket sent:     %s\n", message.c_str());

    notifyClients(message);
}

// Types of responses:
// Info requested
// Event (Button clicked)
// State change (Led on/blinking)