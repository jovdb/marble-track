#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward declaration
class Device;
class DeviceManager;
#include "Network.h"

#include <map>
#include <vector>
#include <functional>
#include <memory>
#include "MessageBatcher.h"

class WebSocketManager
{
private:
    AsyncWebSocket ws;
    DeviceManager *deviceManager;
    Network *network;
    bool scanInProgress = false;
    std::map<uint32_t, String> messageBuffers;

    // Outbound message batching is delegated to MessageBatcher (constructed in
    // the .cpp). The wire format — every send is a JSON array, batched or not
    // — lives there, not here. WebSocketManager just forwards notifyClients/
    // beginBatch/endBatch to the batcher.
    std::unique_ptr<MessageBatcher> batcher;

    // Message-type dispatch table. Built once in the constructor; parseMessage()
    // looks up the incoming "type" field and invokes the matching handler. Adding
    // a new message type means adding one entry here — no edits to parseMessage().
    using MessageHandler = std::function<void(JsonDocument &)>;
    std::map<String, MessageHandler> dispatchTable;

    // Helper methods for cleaner message handling
    void handleRestart();
    void handleDeviceFunction(JsonDocument &doc);
    void handleDeviceState(JsonDocument &doc);
    void handleDeviceGetState(JsonDocument &doc);
    void handleGetDevices(JsonDocument &doc);
    void handleGetSystemInfo(JsonDocument &doc);
    void serializeDeviceToJson(Device *device, JsonObject deviceObj);

public:
    WebSocketManager(DeviceManager *deviceManager, Network *network, const char *path = "/ws");
    void setup(AsyncWebServer &server);
    void loop();
    void notifyClients(String state);
    void beginBatch();
    void endBatch();
    String getStatus() const;
    uint32_t getClientCount() const;
    bool hasClients() const { return ws.count() > 0; }
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void setDeviceManager(DeviceManager *deviceManager);
    void setNetwork(Network *network);

    // Made public to allow global function access
    void parseMessage(String message);

    // Device config handlers
    void handleDeviceSaveConfig(JsonDocument &doc);
    void handleDeviceReadConfig(JsonDocument &doc);
    void handleSetDevicesConfig(JsonDocument &doc);
    void handleGetDevicesConfig(JsonDocument &doc);

    // Device management handlers
    void handleAddDevice(JsonDocument &doc);
    void handleRemoveDevice(JsonDocument &doc);
    void handleReorderDevices(JsonDocument &doc);

    // Network config handlers
    void handleGetNetworkConfig(JsonDocument &doc);
    void handleSetNetworkConfig(JsonDocument &doc);
    void handleGetNetworks(JsonDocument &doc);
    void handleGetNetworkStatus(JsonDocument &doc);

    // I2C handlers
    void handleGetExpanderAddresses(JsonDocument &doc);
};

#endif
