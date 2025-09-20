#include "Logging.h"
#include <LittleFS.h>
#include "WebSocketManager.h"
#include "DeviceManager.h"
#include "NetworkSettings.h"
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

void WebSocketManager::handleGetDevices(JsonDocument &doc)
{
    JsonDocument response;
    response["type"] = "devices-list";

    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
    }
    else
    {
        // Get devices from DeviceManager
        Device *deviceList[20]; // MAX_DEVICES from DeviceManager
        int count;
        deviceManager->getDevices(deviceList, count, 20);

        // Create devices array in JSON
        JsonArray devicesArray = response["devices"].to<JsonArray>();

        for (int i = 0; i < count; i++)
        {
            if (deviceList[i] != nullptr)
            {
                JsonObject deviceObj = devicesArray.add<JsonObject>();
                deviceObj["id"] = deviceList[i]->getId();
                deviceObj["name"] = deviceList[i]->getName();
                deviceObj["type"] = deviceList[i]->getType();

                // Add pins array
                std::vector<int> pins = deviceList[i]->getPins();
                JsonArray pinsArray = deviceObj["pins"].to<JsonArray>();
                for (int pin : pins)
                {
                    pinsArray.add(pin);
                }
            }
        }
    }

    String message;
    serializeJson(response, message);
    // MLOG_WS_SEND("WebSocket sent devices list: %s", message.c_str());

    notifyClients(message);
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
    MLOG_WS_RECEIVE("%s", message.c_str());

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

    if (type == "get-devices")
    {
        handleGetDevices(doc);
        return;
    }

    // Handler for replacing devices.json via websocket upload
    if (type == "set-devices-config")
    {
        JsonDocument response;
        response["type"] = "set-devices-config-result";
        if (!doc["config"].is<JsonArray>())
        {
            response["success"] = false;
            response["error"] = "Missing or invalid config array";
        }
        else
        {
            File file = LittleFS.open("/devices.json", "w");
            if (!file)
            {
                response["success"] = false;
                response["error"] = "Failed to open devices.json for writing";
            }
            else
            {
                serializeJson(doc["config"], file);
                file.close();
                response["success"] = true;
                response["message"] = "devices.json updated";
            }
        }
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // New handler for downloading devices.json config
    if (type == "get-devices-config")
    {
        JsonDocument response;
        response["type"] = "devices-config";
        // Read devices.json from LittleFS
        File file = LittleFS.open("/devices.json", "r");
        if (!file)
        {
            response["error"] = "devices.json not found";
        }
        else
        {
            // Parse file contents as JSON
            JsonDocument configDoc;
            DeserializationError err = deserializeJson(configDoc, file);
            if (err)
            {
                response["error"] = "Failed to parse devices.json";
            }
            else
            {
                response["config"] = configDoc;
            }
            file.close();
        }
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (type == "device-save-config")
    {
        handleDeviceSaveConfig(doc);
        return;
    }
    if (type == "device-read-config")
    {
        handleDeviceReadConfig(doc);
        return;
    }
    if (type == "add-device")
    {
        handleAddDevice(doc);
        return;
    }
    if (type == "remove-device")
    {
        handleRemoveDevice(doc);
        return;
    }
    if (type == "get-network-config")
    {
        handleGetNetworkConfig(doc);
        return;
    }
    if (type == "set-network-config")
    {
        handleSetNetworkConfig(doc);
        return;
    }
    if (type == "get-networks")
    {
        handleGetNetworks(doc);
        return;
    }
}

// Save config from client for a device
void WebSocketManager::handleDeviceSaveConfig(JsonDocument &doc)
{
    String deviceId = doc["deviceId"] | "";
    if (!deviceManager)
    {
        notifyClients(createJsonResponse(false, "DeviceManager not available", "", ""));
        return;
    }
    Device *device = deviceManager->getDeviceById(deviceId);
    if (!device)
    {
        notifyClients(createJsonResponse(false, "Device not found: " + deviceId, "", ""));
        return;
    }
    if (!doc["config"].is<JsonObject>())
    {
        notifyClients(createJsonResponse(false, "No config provided", "", ""));
        return;
    }
    JsonObject configObj = doc["config"].as<JsonObject>();
    device->setConfig(&configObj);

    deviceManager->saveDevicesToJsonFile();
    // notifyClients(createJsonResponse(true, "Config saved", "", ""));

    String configStr = device->getConfig();
    JsonDocument config;
    deserializeJson(config, configStr);

    JsonDocument response;
    response["type"] = "device-config";
    response["deviceId"] = deviceId;
    response["config"] = config;
    String respStr;
    serializeJson(response, respStr);

    notifyClients(respStr);
}

// Read config for a device and send to client
void WebSocketManager::handleDeviceReadConfig(JsonDocument &doc)
{
    String deviceId = doc["deviceId"] | "";
    if (!deviceManager)
    {
        notifyClients(createJsonResponse(false, "DeviceManager not available", "", ""));
        return;
    }
    Device *device = deviceManager->getDeviceById(deviceId);
    if (!device)
    {
        notifyClients(createJsonResponse(false, "Device not found: " + deviceId, "", ""));
        return;
    }

    String configStr = device->getConfig();
    JsonDocument config;
    deserializeJson(config, configStr);

    JsonDocument response;
    response["type"] = "device-config";
    response["deviceId"] = deviceId;
    response["config"] = config;
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
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
        MLOG_INFO("WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());

        // Send welcome message with connection info
        String welcome = "{\"type\":\"connection\",\"message\":\"WebSocket connected\",\"clientId\":" + String(client->id()) + "}";
        client->text(welcome);
        break;
    }

    case WS_EVT_DISCONNECT:
        MLOG_INFO("WebSocket client #%u disconnected", client->id());
        break;

    case WS_EVT_DATA:
        MLOG_INFO("WebSocket client #%u data received", client->id());
        handleWebSocketMessage(arg, data, len);
        break;

    case WS_EVT_PONG:
        MLOG_INFO("WebSocket client #%u pong", client->id());
        break;

    case WS_EVT_ERROR:
        MLOG_ERROR("WebSocket client #%u ERROR occurred", client->id());
        break;
    }
}

WebSocketManager::WebSocketManager(DeviceManager *deviceManager, const char *path)
    : ws(path), deviceManager(deviceManager)
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
    MLOG_INFO("WebSocket manager: OK");
    MLOG_INFO("WebSocket path: /ws");
    MLOG_INFO("WebSocket server ready to accept connections");
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
    MLOG_WS_SEND("%s", state.c_str());
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
    MLOG_INFO("Restarting device...");
    delay(1000);
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
            MLOG_INFO("Executing action: %s[%s]", deviceId.c_str(), functionName.c_str());
            JsonObject payload = doc["args"].as<JsonObject>();
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

void WebSocketManager::handleAddDevice(JsonDocument &doc)
{
    String deviceType = doc["deviceType"] | "";
    String deviceId = doc["deviceId"] | "";
    JsonDocument response;
    response["type"] = "add-device";

    if (deviceType.isEmpty() || deviceId.isEmpty())
    {
        response["error"] = "Missing deviceType or deviceId";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Check if device already exists
    if (deviceManager->getDeviceById(deviceId) != nullptr)
    {
        response["error"] = "Device with ID '" + deviceId + "' already exists";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (!deviceManager->addDevice(deviceType, deviceId, doc["config"]))
    {
        response["error"] = "Failed to create and add device of type '" + deviceType + "' with ID '" + deviceId + "'";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Setup the new device
    Device* newDevice = deviceManager->getDeviceById(deviceId);
    if (newDevice != nullptr) 
    {
        newDevice->setup();
    }
    
    // Save devices to file
    deviceManager->saveDevicesToJsonFile();

    response["success"] = true;
    response["deviceId"] = deviceId;
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
    
    // Broadcast updated device list to all clients
    JsonDocument emptyDoc;
    handleGetDevices(emptyDoc);
    
    MLOG_INFO("Added device: %s (%s)", deviceId.c_str(), deviceType.c_str());
}

void WebSocketManager::handleRemoveDevice(JsonDocument &doc)
{
    String deviceId = doc["deviceId"] | "";
    JsonDocument response;
    response["type"] = "remove-device";

    if (deviceId.isEmpty())
    {
        response["error"] = "Missing deviceId";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (!deviceManager->removeDevice(deviceId))
    {
        response["error"] = "Device not found or failed to remove: " + deviceId;
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Save devices to file
    deviceManager->saveDevicesToJsonFile();

    response["success"] = true;
    response["deviceId"] = deviceId;
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
    
    // Broadcast updated device list to all clients
    JsonDocument emptyDoc;
    handleGetDevices(emptyDoc);
    
    MLOG_INFO("Removed device: %s", deviceId.c_str());
}

void WebSocketManager::handleGetNetworkConfig(JsonDocument &doc)
{
    JsonDocument response;
    response["type"] = "get-network-config";

    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    NetworkSettings settings = deviceManager->loadNetworkSettings();
    
    if (settings.isValid())
    {
        response["ssid"] = settings.ssid;
        // Password is not sent for security reasons
    }
    else
    {
        response["error"] = "No network settings found";
    }

    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
    
    MLOG_INFO("Sent network config to client");
}

void WebSocketManager::handleSetNetworkConfig(JsonDocument &doc)
{
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    
    JsonDocument response;
    response["type"] = "set-network-config";

    if (ssid.isEmpty())
    {
        response["error"] = "SSID cannot be empty";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    NetworkSettings settings(ssid, password);
    if (deviceManager->saveNetworkSettings(settings))
    {
        response["success"] = true;
        MLOG_INFO("Network settings saved: SSID='%s'", ssid.c_str());
    }
    else
    {
        response["error"] = "Failed to save network settings";
    }

    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
}

void WebSocketManager::handleGetNetworks(JsonDocument &doc)
{
    JsonDocument response;
    response["type"] = "get-networks";

    MLOG_INFO("Scanning for available WiFi networks...");

    // Start WiFi scan
    int numNetworks = WiFi.scanNetworks();
    
    if (numNetworks == WIFI_SCAN_FAILED)
    {
        response["error"] = "WiFi scan failed";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    JsonArray networksArray = response["networks"].to<JsonArray>();
    
    for (int i = 0; i < numNetworks; i++)
    {
        JsonObject networkObj = networksArray.add<JsonObject>();
        networkObj["ssid"] = WiFi.SSID(i);
        networkObj["rssi"] = WiFi.RSSI(i);
        networkObj["encryption"] = WiFi.encryptionType(i);
        networkObj["channel"] = WiFi.channel(i);
        networkObj["bssid"] = WiFi.BSSIDstr(i);
        networkObj["hidden"] = WiFi.SSID(i).isEmpty();
    }

    response["count"] = numNetworks;
    
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
    
    MLOG_INFO("Found %d WiFi networks", numNetworks);
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

    notifyClients(message);
}

// Types of responses:
// Info requested
// Event (Button clicked)
// State change (Led on/blinking)