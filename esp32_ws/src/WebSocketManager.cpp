#include "Logging.h"
#include <LittleFS.h>
#include "WebSocketManager.h"
#include "devices/mixins/IControllable.h"
#include "devices/mixins/SerializableMixin.h"
#include "devices/Led.h"
#include "devices/Button.h"
#include "devices/Device.h"
#include "devices/I2c.h"
#include "Config.h"
#include "DeviceManager.h"
#include "Network.h"
#include "NetworkSettings.h"

// Singleton instance pointer used by the C-style AsyncWebSocket event callback.
//
// ESPAsyncWebServer requires the event handler to be a plain function (or a
// captureless lambda), not a member function. To route the callback into the
// WebSocketManager object, a file-scope pointer to the single active instance
// is stored here and dereferenced inside the lambda (see setup()).
//
// Limitation: only one WebSocketManager may be alive at a time. Creating a
// second instance will silently overwrite this pointer, and the first instance
// will stop receiving events. If multiple WebSocket endpoints are ever needed,
// replace this with a map<AsyncWebSocket*, WebSocketManager*>.
static WebSocketManager *instance = nullptr;

namespace
{
    constexpr size_t kMaxQueuedBatchMessages = 64;

    const char *getResetReasonString(esp_reset_reason_t reason)
    {
        switch (reason)
        {
        case ESP_RST_POWERON:
            return "power-on";
        case ESP_RST_EXT:
            return "external-pin";
        case ESP_RST_SW:
            return "software";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt-watchdog";
        case ESP_RST_TASK_WDT:
            return "task-watchdog";
        case ESP_RST_WDT:
            return "other-watchdog";
        case ESP_RST_DEEPSLEEP:
            return "deep-sleep";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_SDIO:
            return "sdio";
        default:
            return "unknown";
        }
    }
}

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

void WebSocketManager::handleGetI2CAddresses(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "i2c-addresses";

    // Get i2cDeviceId from request
    const String i2cDeviceId = doc["i2cDeviceId"] | "";
    if (i2cDeviceId.isEmpty())
    {
        response["error"] = "No I2C device ID specified";
        String message;
        serializeJson(response, message);
        notifyClients(message);
        return;
    }

    // Get I2C device by ID from deviceManager
    devices::I2c *i2cDevice = nullptr;
    if (deviceManager)
    {
        i2cDevice = deviceManager->getDeviceByIdAs<devices::I2c>(i2cDeviceId);
    }

    if (!i2cDevice)
    {
        response["error"] = "I2C device not found: " + i2cDeviceId;
        String message;
        serializeJson(response, message);
        notifyClients(message);
        return;
    }

    // Get I2C pins
    auto i2cPins = i2cDevice->getPins();
    if (i2cPins.size() < 2)
    {
        response["error"] = "I2C device not properly configured";
        String message;
        serializeJson(response, message);
        notifyClients(message);
        return;
    }

    int sdaPin = i2cPins[0].toInt();
    int sclPin = i2cPins[1].toInt();

    // Scan I2C bus for devices (addresses 0x03 to 0x77) using the unified scan method
    std::vector<int> found = i2cDevice->scanBus();
    JsonArray addresses = response["addresses"].to<JsonArray>();
    for (int addr : found)
    {
        addresses.add(addr);
    }
    int deviceCount = found.size();

    String message;
    serializeJson(response, message);
    MLOG_INFO("Found %d I2C devices on bus '%s' (SDA=%d, SCL=%d)", deviceCount, i2cDeviceId.c_str(), sdaPin, sclPin);
    notifyClients(message);
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
        Device *deviceList[20];
        int count;
        deviceManager->getDevices(deviceList, count, 20);

        // Create devices array in JSON
        JsonArray devicesArray = response["devices"].to<JsonArray>();

        for (int i = 0; i < count; i++)
        {
            if (deviceList[i])
            {
                // Skip devices that are single children (have exactly one child with no children)
                auto children = deviceList[i]->getChildren();
                bool isSingleChildDevice = (children.size() == 1) && (children[0]->getChildren().empty());
                if (isSingleChildDevice)
                {
                    continue; // Skip this device, it will be included as a child of its parent
                }

                // Recursively serialize this device and its children
                serializeDeviceToJson(deviceList[i], devicesArray.add<JsonObject>());
            }
        }
    }

    String message;
    serializeJson(response, message);

    notifyClients(message);
}

void WebSocketManager::handleGetSystemInfo(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "system-info";
    response["serialBaudRate"] = Config::SERIAL_BAUD_RATE;
    response["firmwareBuild"] = String(__DATE__) + " " + String(__TIME__);
    response["freeHeap"] = ESP.getFreeHeap();
    response["uptimeMs"] = millis();
    response["webSocketClients"] = ws.count();
    response["resetReason"] = getResetReasonString(esp_reset_reason());
    response["chipModel"] = ESP.getChipModel();
    response["sdkVersion"] = ESP.getSdkVersion();

    if (network)
    {
        response["hostname"] = network->getHostname();
        response["ipAddress"] = network->getIPAddress().toString();
        response["connectionInfo"] = network->getConnectionInfo();
    }

    String message;
    serializeJson(response, message);
    notifyClients(message);

    MLOG_INFO("Sent system info to client");
}

/**
 * @brief Recursively serialize a device and its children to JSON
 * @param device The device to serialize
 * @param deviceObj The JSON object to populate
 */
void WebSocketManager::serializeDeviceToJson(Device *device, JsonObject deviceObj)
{
    if (!device)
        return;

    deviceObj["id"] = device->getId();
    deviceObj["type"] = device->getType();

    // Ensure devices-list stays lean; config/state are delivered via device-config/device-state
    deviceObj.remove("config");
    deviceObj.remove("state");

    // Add pins array
    std::vector<String> pins = device->getPins();
    JsonArray pinsArr = deviceObj["pins"].to<JsonArray>();
    for (String pin : pins)
    {
        pinsArr.add(pin);
    }

    // Generic features: mirror mixins as an array
    {
        const auto &mixins = device->getMixins();
        JsonArray featuresArr = deviceObj["features"].to<JsonArray>();
        for (const auto &mx : mixins)
        {
            featuresArr.add(mx);
        }
    }

    // Add children array recursively
    JsonArray childrenArr = deviceObj["children"].to<JsonArray>();
    for (Device *child : device->getChildren())
    {
        if (child)
        {
            JsonObject childObj = childrenArr.add<JsonObject>();
            serializeDeviceToJson(child, childObj);
        }
    }
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

    // Look up the handler for this message type and invoke it. Unknown types
    // are silently ignored (matching the previous behavior of the if-chain).
    auto it = dispatchTable.find(type);
    if (it != dispatchTable.end())
    {
        it->second(doc);
    }
    else
    {
        MLOG_WARN("Unknown WebSocket message type: '%s'", type.c_str());
    }
}

// Save config from client for a device
void WebSocketManager::handleDeviceSaveConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    static constexpr const char *kType = "device-save-config";
    const String deviceId = doc["deviceId"] | "";
    if (!deviceManager)
    {
        notifyClients(createJsonResponse(false, "DeviceManager not available", "", "", kType, deviceId));
        return;
    }

    // Support composition devices (Device) that implement serializable config
    Device *device = deviceManager->getDeviceById(deviceId);
    if (device)
    {
        if (!doc["config"].is<JsonObject>())
        {
            notifyClients(createJsonResponse(false, "No config provided", "", "", kType, deviceId));
            return;
        }

        // Use registry to get serializable interface - works for any device type
        if (device->hasMixin("serializable"))
        {
            ISerializable *serializable = mixins::SerializableRegistry::get(deviceId);
            if (serializable)
            {
                // Apply the incoming config to the device
                JsonDocument configDoc;
                configDoc.set(doc["config"].as<JsonObject>());

                // Restart from the root parent so that parent state (e.g. Wheel's
                // lastZeroPosition) is fully reset when a child (e.g. Stepper) is
                // reconfigured.  If `device` is already a root, root == device and
                // the behaviour is identical to before.
                Device *root = deviceManager->getRootDeviceOf(device);
                root->teardown();
                serializable->jsonToConfig(configDoc);
                root->setup();

                deviceManager->saveDevicesToJsonFile();

                // Build device-config response after save
                JsonDocument response;
                response["type"] = "device-config";
                response["triggerBy"] = "set";
                response["deviceId"] = deviceId;

                JsonDocument savedConfig;
                serializable->configToJson(savedConfig);
                if (savedConfig.overflowed())
                {
                    MLOG_ERROR("device-save-config: config JSON overflowed for %s", deviceId.c_str());
                }

                response["config"] = savedConfig;

                if (response.overflowed())
                {
                    MLOG_ERROR("device-save-config: response JSON overflowed for %s", deviceId.c_str());
                }
                String respStr;
                serializeJson(response, respStr);
                notifyClients(respStr);

                deviceManager->notifyDevicesChanged();

                // Send updated device information after config change and setup
                beginBatch();

                // Send updated devices list
                JsonDocument emptyDoc;
                handleGetDevices(emptyDoc);

                std::vector<Device *> allDevices = deviceManager->getAllDevices();

                // Send config for all devices
                for (Device *dev : allDevices)
                {
                    JsonDocument configDoc;
                    configDoc["deviceId"] = dev->getId();
                    handleDeviceReadConfig(configDoc);
                }

                // Send state for all devices
                for (Device *dev : allDevices)
                {
                    JsonDocument stateDoc;
                    stateDoc["deviceId"] = dev->getId();
                    handleDeviceGetState(stateDoc);
                }

                endBatch();

                return;
            }
        }

        // Device exists but doesn't support serializable
        notifyClients(createJsonResponse(false, "Device does not support config: " + device->getType(), "", "", kType, deviceId));
        return;
    }

    notifyClients(createJsonResponse(false, "Device not found: " + deviceId, "", "", kType, deviceId));
}

// Read config for a device and send to client
void WebSocketManager::handleDeviceReadConfig(JsonDocument &doc)
{
    if (!hasClients())
        return;

    static constexpr const char *kType = "device-read-config";
    const String deviceId = doc["deviceId"] | "";

    if (!deviceManager)
    {
        String response = createJsonResponse(false, "DeviceManager not available", "", "", kType, deviceId);
        notifyClients(response);
        return;
    }

    Device *device = deviceManager->getDeviceById(deviceId);
    if (device)
    {
        JsonDocument response;
        response["type"] = "device-config";
        response["triggerBy"] = "get";
        response["deviceId"] = deviceId;

        // Use registry to get serializable interface - works for any device type
        if (device->hasMixin("serializable"))
        {
            ISerializable *serializable = mixins::SerializableRegistry::get(deviceId);
            if (serializable)
            {
                JsonDocument configDoc;
                serializable->configToJson(configDoc);

                if (configDoc.overflowed())
                {
                    MLOG_ERROR("Config JSON overflowed for device %s", deviceId.c_str());
                }

                response["config"] = configDoc;
            }
            else
            {
                response["config"] = nullptr;
            }
        }
        else
        {
            response["config"] = nullptr;
        }

        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    MLOG_ERROR("Device not found for config read request: %s", deviceId.c_str());
    String response = createJsonResponse(false, "Device not found: " + deviceId, "", "", kType, deviceId);
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
        // Write to a temp file first so a failed write never corrupts the live config.
        static constexpr const char *TMP_FILE = "/config.json.tmp";
        static constexpr const char *BAK_FILE = "/config.json.bak";
        File file = LittleFS.open(TMP_FILE, "w");
        if (!file)
        {
            response["success"] = false;
            response["error"] = "Failed to open temp config file for writing";
        }
        else
        {
            size_t written = serializeJson(doc["config"], file);
            file.close();

            if (written == 0)
            {
                LittleFS.remove(TMP_FILE);
                response["success"] = false;
                response["error"] = "serializeJson wrote 0 bytes (storage full?)";
            }
            else
            {
                // Promote: backup current config, then rename temp → live.
                if (LittleFS.exists("/config.json"))
                {
                    LittleFS.remove(BAK_FILE);
                    LittleFS.rename("/config.json", BAK_FILE);
                }
                LittleFS.rename(TMP_FILE, "/config.json");
                response["success"] = true;
                response["message"] = "config.json updated";

                MLOG_INFO("Config written to file, reloading devices");
                // Atomic reload: teardown -> load -> setup -> notifyDevicesChanged.
                // See DeviceManager::reloadFromJsonFile() for the ordering rationale.
                if (deviceManager)
                {
                    deviceManager->reloadFromJsonFile();
                }
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
    if (!deviceManager)
    {
        response["error"] = "DeviceManager not available";
    }
    else
    {
        // Build a config snapshot that mirrors saveDevicesToJsonFile()
        JsonDocument configDoc;
        JsonArray devicesArray = configDoc["devices"].to<JsonArray>();
        deviceManager->addDevicesToJsonArray(devicesArray);
        response["config"] = configDoc;
    }

    if (response.overflowed())
    {
        MLOG_ERROR("devices-config JSON overflowed!");
    }

    String respStr;
    serializeJson(response, respStr);
    MLOG_DEBUG("devices-config serialized: %d bytes", respStr.length());
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
        // Erase any partial multi-frame message that was being accumulated
        // for this client; leaving it would be a permanent memory leak because
        // the client id may be reused for a different connection later.
        messageBuffers.erase(client->id());
        break;

    case WS_EVT_DATA:
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            if (info->final && info->index == 0 && info->len == len)
            {
                // Single frame message.
                // Use the explicit-length String constructor instead of
                // null-terminating data[len] — that write is one byte past
                // the buffer the library allocated (valid indices 0..len-1).
                String message((char *)data, len);
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
    : ws(path), deviceManager(deviceManager), network(network)
{
    instance = this;

    // Initialize the message batcher. The transport callbacks capture `this`
    // and forward to the AsyncWebSocket; all wire-format concerns live in
    // MessageBatcher.
    batcher.reset(new MessageBatcher(
        [this](const String &payload)
        { ws.textAll(payload); },
        [this]()
        { return ws.availableForWriteAll(); },
        kMaxQueuedBatchMessages));

    // Build the message-type dispatch table. Each entry maps the wire "type"
    // string to the handler method. Adding a new message type means adding one
    // line here; parseMessage() does not need to change.
    dispatchTable["restart"] = [this](JsonDocument &)
    { handleRestart(); };
    dispatchTable["device-fn"] = [this](JsonDocument &d)
    { handleDeviceFunction(d); };
    dispatchTable["device-state"] = [this](JsonDocument &d)
    { handleDeviceGetState(d); };
    dispatchTable["devices-all-states"] = [this](JsonDocument &d)
    { handleGetAllDeviceStates(d); };
    dispatchTable["devices-list"] = [this](JsonDocument &d)
    { handleGetDevices(d); };
    dispatchTable["system-info"] = [this](JsonDocument &d)
    { handleGetSystemInfo(d); };
    dispatchTable["set-devices-config"] = [this](JsonDocument &d)
    { handleSetDevicesConfig(d); };
    dispatchTable["devices-config"] = [this](JsonDocument &d)
    { handleGetDevicesConfig(d); };
    dispatchTable["device-save-config"] = [this](JsonDocument &d)
    { handleDeviceSaveConfig(d); };
    dispatchTable["device-read-config"] = [this](JsonDocument &d)
    { handleDeviceReadConfig(d); };
    dispatchTable["add-device"] = [this](JsonDocument &d)
    { handleAddDevice(d); };
    dispatchTable["remove-device"] = [this](JsonDocument &d)
    { handleRemoveDevice(d); };
    dispatchTable["reorder-devices"] = [this](JsonDocument &d)
    { handleReorderDevices(d); };
    dispatchTable["network-config"] = [this](JsonDocument &d)
    { handleGetNetworkConfig(d); };
    dispatchTable["set-network-config"] = [this](JsonDocument &d)
    { handleSetNetworkConfig(d); };
    dispatchTable["networks"] = [this](JsonDocument &d)
    { handleGetNetworks(d); };
    dispatchTable["network-status"] = [this](JsonDocument &d)
    { handleGetNetworkStatus(d); };
    dispatchTable["i2c-addresses"] = [this](JsonDocument &d)
    { handleGetI2CAddresses(d); };
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

uint32_t WebSocketManager::getClientCount() const
{
    return ws.count();
}

void WebSocketManager::notifyClients(String state)
{
    if (!hasClients())
        return;
    batcher->send(state);
}

void WebSocketManager::beginBatch()
{
    batcher->beginBatch();
}

void WebSocketManager::endBatch()
{
    if (!hasClients())
    {
        // Still end the batch (clears state) but skip the flush.
        batcher->endBatch();
        return;
    }
    batcher->endBatch();
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
    if (device && device->hasMixin("controllable"))
    {
        IControllable *ctrl = mixins::ControllableRegistry::get(deviceId);
        if (ctrl)
        {
            JsonObject *payloadPtr = nullptr;
            JsonObject payloadObj;
            if (doc["args"].is<JsonObject>())
            {
                payloadObj = doc["args"].as<JsonObject>();
                payloadPtr = &payloadObj;
            }
            MLOG_INFO("%s: Starting action '%s'.", device->toString().c_str(), functionName.c_str());
            ctrl->control(functionName, payloadPtr);
            return;
        }
        else
        {
            MLOG_WARN("handleDeviceFunction: Controllable mixin registry has no interface for %s", deviceId.c_str());
            return;
        }
    }
    else
    {
        MLOG_WARN("handleDeviceFunction: Device not found or not controllable: %s", deviceId.c_str());
    }
}

void WebSocketManager::handleGetAllDeviceStates(JsonDocument &doc)
{
    if (!hasClients() || !deviceManager)
        return;

    // Collect all devices (roots + children at every depth).
    std::vector<Device *> allDevices = deviceManager->getAllDevices();

    // Wrap every per-device state response in one batch so the entire
    // snapshot goes out in a single WebSocket frame and never overflows
    // the send buffer on reconnection.
    beginBatch();
    for (Device *device : allDevices)
    {
        if (!device)
            continue;

        const String deviceId = device->getId();

        JsonDocument responseDoc;
        responseDoc["type"] = "device-state";
        responseDoc["success"] = true;
        responseDoc["deviceId"] = deviceId;

        if (device->hasMixin("controllable"))
        {
            IControllable *ctrl = mixins::ControllableRegistry::get(deviceId);
            if (ctrl)
            {
                JsonDocument stateDoc;
                ctrl->addStateToJson(stateDoc);
                responseDoc["state"] = stateDoc;
            }
            else
            {
                responseDoc["state"] = nullptr;
            }
        }
        else
        {
            responseDoc["state"] = nullptr;
        }

        String response;
        serializeJson(responseDoc, response);
        notifyClients(response);
    }
    endBatch();
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
        // If the device implements the controllable mixin, return its JSON state
        if (device->hasMixin("controllable"))
        {
            // Lookup controllable interface via global registry (base remains agnostic)
            IControllable *ctrl = mixins::ControllableRegistry::get(deviceId);
            if (ctrl)
            {
                JsonDocument stateDoc;
                ctrl->addStateToJson(stateDoc);

                JsonDocument responseDoc;
                responseDoc["type"] = "device-state";
                responseDoc["success"] = true;
                responseDoc["deviceId"] = deviceId;
                responseDoc["state"] = stateDoc;

                String response;
                serializeJson(responseDoc, response);
                notifyClients(response);
                return;
            }
        }
        // Device exists but is not controllable or no interface: return null state
        JsonDocument responseDoc;
        responseDoc["type"] = "device-state";
        responseDoc["success"] = true;
        responseDoc["deviceId"] = deviceId;
        responseDoc["state"] = nullptr;

        String response;
        serializeJson(responseDoc, response);
        notifyClients(response);
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
    if (deviceManager->getDeviceById(deviceId))
    {
        response["error"] = "Device with ID '" + deviceId + "' already exists";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Create device using the factory
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
    if (newDevice)
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

    // MLOG_INFO("Added device: %s (%s)", deviceId.c_str(), deviceType.c_str());
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

void WebSocketManager::handleReorderDevices(JsonDocument &doc)
{
    if (!hasClients())
        return;

    JsonDocument response;
    response["type"] = "reorder-devices";

    if (!doc["deviceIds"].is<JsonArray>())
    {
        response["error"] = "Missing or invalid deviceIds array";
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

    // Extract device IDs from JSON array
    JsonArray deviceIdsArray = doc["deviceIds"].as<JsonArray>();
    std::vector<String> deviceIds;
    for (JsonVariant id : deviceIdsArray)
    {
        if (id.is<const char *>())
        {
            deviceIds.push_back(id.as<String>());
        }
    }

    if (deviceIds.empty())
    {
        response["error"] = "No valid device IDs provided";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Reorder devices
    if (!deviceManager->reorderDevices(deviceIds))
    {
        response["error"] = "Failed to reorder devices";
        String respStr;
        serializeJson(response, respStr);
        notifyClients(respStr);
        return;
    }

    // Save the new order to file
    deviceManager->saveDevicesToJsonFile();

    response["success"] = true;
    String respStr;
    serializeJson(response, respStr);
    notifyClients(respStr);

    // Broadcast updated device list to all clients
    JsonDocument emptyDoc;
    handleGetDevices(emptyDoc);

    MLOG_INFO("Reordered devices");
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
        MLOG_INFO("Network settings saved.");
    }
    else
    {
        response["success"] = false;
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
