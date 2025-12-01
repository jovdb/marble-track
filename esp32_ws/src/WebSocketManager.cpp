#include "Logging.h"
#include <LittleFS.h>
#include "WebSocketManager.h"
#include "DeviceManager.h"
#include "Network.h"
#include "NetworkSettings.h"
#include "TimeManager.h"

// Static instance for callback access (simplified to single instance)
static WebSocketManager *instance = nullptr;

String createJsonResponse(bool success, const String &message, const String &data, const String &requestId, const String &type = "", const String &deviceId = "")
{
    JsonDocument response;
    response["success"] = success;
    response["message"] = message;

    if (requestId.length() > 0)
    {
        response["requestId"] = requestId;
    }

    if (type.length() > 0)
    {
        response["type"] = type;
    }

    if (deviceId.length() > 0)
    {
        response["deviceId"] = deviceId;
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
    if (!hasClients())
        return;

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
                // Skip devices that are single children (have exactly one child with no children)
                auto children = deviceList[i]->getChildren();
                bool isSingleChildDevice = (children.size() == 1) && (children[0]->getChildren().empty());
                if (isSingleChildDevice)
                {
                    continue; // Skip this device, it will be included as a child of its parent
                }

                JsonObject deviceObj = devicesArray.add<JsonObject>();
                deviceObj["id"] = deviceList[i]->getId();
                deviceObj["type"] = deviceList[i]->getType();

                // Add pins array
                std::vector<int> pins = deviceList[i]->getPins();
                JsonArray pinsArr = deviceObj["pins"].to<JsonArray>();
                for (int pin : pins)
                {
                    pinsArr.add(pin);
                }

                // Add children array
                JsonArray childrenArr = deviceObj["children"].to<JsonArray>();
                for (Device *child : deviceList[i]->getChildren())
                {
                    if (child)
                    {
                        JsonObject childObj = childrenArr.add<JsonObject>();
                        childObj["id"] = child->getId();
                        childObj["type"] = child->getType();

                        // Add child pins
                        std::vector<int> childPins = child->getPins();
                        JsonArray childPinsArr = childObj["pins"].to<JsonArray>();
                        for (int pin : childPins)
                        {
                            childPinsArr.add(pin);
                        }
                    }
                }
            }
        }

        // Get task devices from DeviceManager
        TaskDevice *taskDeviceList[10]; // MAX_TASK_DEVICES from DeviceManager
        int taskCount;
        deviceManager->getTaskDevices(taskDeviceList, taskCount, 10);

        for (int i = 0; i < taskCount; i++)
        {
            if (taskDeviceList[i] != nullptr)
            {
                JsonObject deviceObj = devicesArray.add<JsonObject>();
                deviceObj["id"] = taskDeviceList[i]->getId();
                deviceObj["type"] = taskDeviceList[i]->getType();

                // Get pins directly from TaskDevice (now virtual)
                std::vector<int> pins = taskDeviceList[i]->getPins();
                JsonArray pinsArr = deviceObj["pins"].to<JsonArray>();
                for (int pin : pins)
                {
                    pinsArr.add(pin);
                }
            }
        }
    }

    String message;
    serializeJson(response, message);

    notifyClients(message);
}

/**
 * @brief Parse and handle incoming WebSocket messages
 */
void WebSocketManager::parseMessage(String message)

{
    MLOG_WS_RECEIVE("%s", message.c_str());

    // Parse as JSON
    JsonDocument doc; // Dynamic sizing for large messages
    if (deserializeJson(doc, message))
    {
        if (hasClients())
        {
            String errorResponse = createJsonResponse(false, "Invalid JSON format", "", "");
            notifyClients(errorResponse);
        }
        return;
    }

    // Extract type (check both root and data field)
    String type = doc["type"] | "";
    if (type.isEmpty() && doc["data"].is<JsonObject>())
    {
        type = doc["data"]["type"] | "";
    }

    MLOG_WS_RECEIVE("Parsed message type: %s", type.c_str());

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
    if (type == "device-state")
    {
        handleDeviceGetState(doc);
        return;
    }

    if (type == "devices-list")
    {
        handleGetDevices(doc);
        return;
    }

    // Handler for replacing config.json via websocket upload
    if (type == "set-devices-config")
    {
        handleSetDevicesConfig(doc);
        return;
    }

    // New handler for downloading devices.json config
    if (type == "devices-config")
    {
        handleGetDevicesConfig(doc);
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
    if (type == "network-config")
    {
        handleGetNetworkConfig(doc);
        return;
    }
    if (type == "set-network-config")
    {
        handleSetNetworkConfig(doc);
        return;
    }
    if (type == "networks")
    {
        handleGetNetworks(doc);
        return;
    }
    if (type == "network-status")
    {
        handleGetNetworkStatus(doc);
        return;
    }
}

// Save config from client for a device
void WebSocketManager::handleDeviceSaveConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    const String deviceId = doc["deviceId"] | "";
    if (!deviceManager)
    {
        notifyClients(createJsonResponse(false, "DeviceManager not available", "", "", "device-save-config", deviceId));
        return;
    }
    Device *device = deviceManager->getDeviceById(deviceId);
    if (!device)
    {
        notifyClients(createJsonResponse(false, "Device not found: " + deviceId, "", "", "device-save-config", deviceId));
        return;
    }
    if (!doc["config"].is<JsonObject>())
    {
        notifyClients(createJsonResponse(false, "No config provided", "", "", "device-save-config", deviceId));
        return;
    }
    JsonObject configObj = doc["config"].as<JsonObject>();
    device->setConfig(&configObj);

    deviceManager->saveDevicesToJsonFile();
    // notifyClients(createJsonResponse(true, "Config saved", "", ""));

    String configStr = device->getConfig();

    JsonDocument response;
    response["type"] = "device-config";
    response["triggerBy"] = "set";
    response["deviceId"] = deviceId;
    if (configStr.length() > 0)
    {
        JsonDocument configDoc;
        DeserializationError err = deserializeJson(configDoc, configStr);
        if (!err && configDoc.is<JsonObject>())
        {
            response["config"] = configDoc.as<JsonObject>();
        }
        else
        {
            if (err)
            {
                MLOG_WARN("Device %s: failed to deserialize config after save (%s)", deviceId.c_str(), err.c_str());
            }
            else
            {
                MLOG_WARN("Device %s: config after save is not a JSON object, returning raw string", deviceId.c_str());
            }
            response["config"] = serialized(configStr.c_str());
        }
    }
    else
    {
        response["config"].to<JsonObject>();
    }
    String respStr;
    serializeJson(response, respStr);

    notifyClients(respStr);
}

// Read config for a device and send to client
void WebSocketManager::handleDeviceReadConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    const String deviceId = doc["deviceId"] | "";

    if (!deviceManager)
    {
        String response = createJsonResponse(false, "DeviceManager not available", "", "", "device-read-config", deviceId);
        notifyClients(response);
        return;
    }

    Device *device = deviceManager->getDeviceById(deviceId);
    if (device)
    {
        String configStr = device->getConfig();

        JsonDocument response;
        response["type"] = "device-config";
        response["triggerBy"] = "get";
        response["deviceId"] = deviceId;
        if (configStr.length() > 0)
        {
            JsonDocument configDoc;
            DeserializationError err = deserializeJson(configDoc, configStr);
            if (!err && configDoc.is<JsonObject>())
            {
                response["config"] = configDoc.as<JsonObject>();
            }
            else
            {
                if (err)
                {
                    MLOG_WARN("Device %s: failed to deserialize config on read (%s)", deviceId.c_str(), err.c_str());
                }
                else
                {
                    MLOG_WARN("Device %s: config read is not a JSON object, returning raw string", deviceId.c_str());
                }
                response["config"] = serialized(configStr.c_str());
            }
        }
        else
        {
            response["config"].to<JsonObject>();
        }
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    ControllableTaskDevice *controllableDevice = deviceManager->getControllableTaskDeviceById(deviceId);
    if (controllableDevice)
    {
        controllableDevice->notifyConfig(false);
        return;
    }

    MLOG_ERROR("Device not found for config read request: %s", deviceId.c_str());
    String response = createJsonResponse(false, "Device not found: " + deviceId, "", "", "device-read-config", deviceId);
    notifyClients(response);
}

void WebSocketManager::handleSetDevicesConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    MLOG_INFO("Received set-devices-config via WebSocket");
    JsonDocument response;
    response["type"] = "set-devices-config";
    if (!doc["config"].is<JsonObject>())
    {
        response["success"] = false;
        response["error"] = "Missing or invalid config object";
    }
    else
    {
        MLOG_INFO("Config object found, attempting to write to file");
        File file = LittleFS.open("/config.json", "w");
        if (!file)
        {
            response["success"] = false;
            response["error"] = "Failed to open config.json for writing";
        }
        else
        {
            serializeJson(doc["config"], file);
            file.close();
            response["success"] = true;
            response["message"] = "config.json updated";

            MLOG_INFO("Config written to file, reloading devices");
            // Reload devices from the new config
            if (deviceManager)
            {
                deviceManager->loadDevicesFromJsonFile();
                // Broadcast updated device list to all clients
                // JsonDocument emptyDoc;
                // handleGetDevices(emptyDoc);

                deviceManager->notifyDevicesChanged();
            }
        }
    }
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
}

void WebSocketManager::handleGetDevicesConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "devices-config";
    // Read config.json from LittleFS
    File file = LittleFS.open("/config.json", "r");
    if (!file)
    {
        response["error"] = "config.json not found";
    }
    else
    {
        // Parse file contents as JSON
        JsonDocument configDoc;
        DeserializationError err = deserializeJson(configDoc, file);
        if (err)
        {
            response["error"] = "Failed to parse config.json";
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
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            if (info->final && info->index == 0 && info->len == len)
            {
                // Single frame message
                data[len] = 0;
                String message = (char *)data;
                parseMessage(message);
            }
            else
            {
                // Multi-frame message
                if (info->index == 0)
                {
                    messageBuffers[client->id()] = String((char *)data, len);
                }
                else
                {
                    auto it = messageBuffers.find(client->id());
                    if (it != messageBuffers.end())
                    {
                        it->second += String((char *)data, len);
                    }
                }
                if (info->final)
                {
                    auto it = messageBuffers.find(client->id());
                    if (it != messageBuffers.end())
                    {
                        String message = it->second;
                        messageBuffers.erase(it);
                        parseMessage(message);
                    }
                }
            }
        }
        break;
    }

    case WS_EVT_PONG:
        MLOG_INFO("WebSocket client #%u pong", client->id());
        break;

    case WS_EVT_ERROR:
        MLOG_ERROR("WebSocket client #%u ERROR occurred", client->id());
        break;
    }
}

WebSocketManager::WebSocketManager(DeviceManager *deviceManager, Network *network, const char *path)
    : ws(path), deviceManager(deviceManager), network(network), batchingActive(false)
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

    // Check if async WiFi scan is complete
    if (scanInProgress)
    {
        int16_t numNetworks = WiFi.scanComplete();
        if (numNetworks >= 0)
        {
            scanInProgress = false;

            if (!hasClients())
                return;

            JsonDocument response;
            response["type"] = "networks";

            if (numNetworks == WIFI_SCAN_FAILED)
            {
                response["error"] = "WiFi scan failed";
            }
            else
            {
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
                MLOG_INFO("Found %d WiFi networks", numNetworks);
            }

            String respStr;
            serializeJson(response, respStr);
            notifyClients(respStr);
        }
    }
}

String WebSocketManager::getStatus() const
{
    return "{\"connectedClients\":" + String(ws.count()) + ",\"path\":\"/ws\"}";
}

void WebSocketManager::notifyClients(String state)
{
    if (!hasClients())
        return;

    if (batchingActive)
    {
        // Queue message for batch sending
        messageQueue.push_back(state);
    }
    else
    {
        // Send immediately as array
        String arrayMessage = "[" + state + "]";
        MLOG_WS_SEND("%s", arrayMessage.c_str());
        ws.textAll(arrayMessage);
    }
}

void WebSocketManager::beginBatch()
{
    batchingActive = true;
    messageQueue.clear();
}

void WebSocketManager::endBatch()
{
    batchingActive = false;

    if (!hasClients() || messageQueue.empty())
    {
        messageQueue.clear();
        return;
    }

    // Always send as array, even for single messages
    String batchMessage = "[";

    bool firstMessage = true;
    for (size_t i = 0; i < messageQueue.size(); i++)
    {
        // Skip empty messages to prevent double commas
        if (messageQueue[i].isEmpty())
        {
            continue;
        }

        if (!firstMessage)
        {
            batchMessage += ",";
        }
        firstMessage = false;

        MLOG_WS_SEND("%s", messageQueue[i].c_str());
        batchMessage += messageQueue[i];
    }

    batchMessage += "]";

    ws.textAll(batchMessage);

    messageQueue.clear();
}

void WebSocketManager::setDeviceManager(DeviceManager *deviceManager)
{
    this->deviceManager = deviceManager;
}

void WebSocketManager::setNetwork(Network *network)
{
    this->network = network;
}

void WebSocketManager::handleRestart()
{
    if (!hasClients())
    {
        ESP.restart();
        return;
    }

    String response = createJsonResponse(true, "Device restart initiated", "", "");
    notifyClients(response);
    MLOG_INFO("Restarting device...");
    delay(1000);
    ESP.restart();
}

void WebSocketManager::handleDeviceFunction(JsonDocument &doc)
{
    if (!hasClients())
        return;

    // Extract device info from either root or data field
    String deviceId = doc["deviceId"] | "";
    String functionName = doc["fn"] | "";

    if (deviceId.isEmpty() && doc["data"].is<JsonObject>())
    {
        JsonObject dataObj = doc["data"];
        deviceId = dataObj["deviceId"] | "";
        functionName = dataObj["fn"] | "";
    }

    if (!deviceManager)
    {
        String response = createJsonResponse(false, "DeviceManager not available", "", "", "device-fn", deviceId);
        notifyClients(response);
        return;
    }

    Device *device = deviceManager->getDeviceById(deviceId);
    if (device)
    {
        MLOG_INFO("%s: Executing action", deviceId.c_str());
        JsonObject payload = doc["args"].as<JsonObject>();
        device->control(functionName, &payload);
        return;
    }

    ControllableTaskDevice *controllableDevice = deviceManager->getControllableTaskDeviceById(deviceId);
    if (controllableDevice)
    {
        MLOG_INFO("%s: Executing action on controllable device", controllableDevice->toString().c_str());
        JsonObject payload = doc["args"].as<JsonObject>();
        controllableDevice->control(functionName, &payload);
        return;
    }
}

void WebSocketManager::handleDeviceGetState(JsonDocument &doc)
{
    if (!hasClients())
        return;

    // Extract device info from either root or data field
    String deviceId = doc["deviceId"] | "";

    if (!deviceManager)
    {
        String response;
        response = createJsonResponse(false, "No DeviceManager available", "", "device-state", deviceId);
        notifyClients(response);
        return;
    }

    Device *device = deviceManager->getDeviceById(deviceId);
    if (device)
    {
        device->notifyStateChange();
        return;
    }

    ControllableTaskDevice *controllableDevice = deviceManager->getControllableTaskDeviceById(deviceId);
    if (controllableDevice)
    {
        controllableDevice->notifyState(false);
        return;
    }

    MLOG_ERROR("Device not found for state request: %s", deviceId.c_str());
    String response;
    response = createJsonResponse(false, "Device not found or not controllable: " + deviceId, "", "", "device-state", deviceId);
    notifyClients(response);
}

void WebSocketManager::handleAddDevice(JsonDocument &doc)
{
    if (!hasClients())
        return;

    const String deviceType = doc["deviceType"] | "";
    const String deviceId = doc["deviceId"] | "";
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
    Device *newDevice = deviceManager->getDeviceById(deviceId);
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

    deviceManager->notifyDevicesChanged();

    MLOG_INFO("Added device: %s (%s)", deviceId.c_str(), deviceType.c_str());
}

void WebSocketManager::handleRemoveDevice(JsonDocument &doc)
{
    if (!hasClients())
        return;

    const String deviceId = doc["deviceId"] | "";
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

    deviceManager->notifyDevicesChanged();

    MLOG_INFO("Removed device: %s", deviceId.c_str());
}

void WebSocketManager::handleGetNetworkConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "network-config";

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
    if (!hasClients())
        return;

    const String ssid = doc["ssid"] | "";
    const String password = doc["password"] | "";

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

    if (!network)
    {
        response["error"] = "Network not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    NetworkSettings settings(ssid, password);

    // Apply network settings immediately, regardless of config save success
    NetworkMode newMode = network->applySettings(settings);
    MLOG_INFO("Network settings applied: SSID='%s', Mode=%s", ssid.c_str(),
              newMode == NetworkMode::WIFI_CLIENT ? "WiFi Client" : newMode == NetworkMode::ACCESS_POINT ? "Access Point"
                                                                                                         : "Disconnected");

    // Try to save settings to config file
    bool saveSuccess = false;
    if (deviceManager)
    {
        saveSuccess = deviceManager->saveNetworkSettings(settings);
    }

    if (saveSuccess)
    {
        response["success"] = true;
        response["message"] = "Network settings updated and saved";
        MLOG_INFO("Network settings saved to config file");
    }
    else
    {
        response["success"] = true;
        response["message"] = "Network settings updated but failed to save to config file";
        response["warning"] = "Settings will not persist after restart";
        MLOG_WARN("Network settings applied but failed to save to config file");
    }

    // Notify clients with updated network config
    JsonDocument emptyDoc;
    handleGetNetworkConfig(emptyDoc);

    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);
}

void WebSocketManager::handleGetNetworks(JsonDocument &doc)
{
    if (!hasClients())
        return;

    if (scanInProgress)
    {
        JsonDocument response;
        response["type"] = "networks";
        response["error"] = "Scan already in progress";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    MLOG_INFO("Starting async WiFi network scan...");

    scanInProgress = true;
    WiFi.scanNetworks(true); // Start async scan
}

void WebSocketManager::handleGetNetworkStatus(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "network-status";

    if (!network)
    {
        response["error"] = "Network not available";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    String statusJson = network->getStatusJSON();

    // Parse the status JSON and add it to the response
    JsonDocument statusDoc;
    if (deserializeJson(statusDoc, statusJson) == DeserializationError::Ok)
    {
        response["status"] = statusDoc.as<JsonObject>();
    }
    else
    {
        response["error"] = "Failed to parse network status";
    }

    String respStr;
    serializeJson(response, respStr);

    notifyClients(respStr);

    MLOG_INFO("Sent network status to client");
}

// Types of responses:
// Info requested
// Event (Button clicked)
// State change (Led on/blinking)