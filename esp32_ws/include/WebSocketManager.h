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

class WebSocketManager
{
private:
    AsyncWebSocket ws;
    DeviceManager *deviceManager;
    Network *network;
    bool scanInProgress = false;
    std::map<uint32_t, String> messageBuffers;
    
    // Message batching
    //
    // Wire format: every outbound WebSocket message is a JSON *array*.
    // Outside a batch each call to notifyClients() sends a single-element
    // array "[<msg>]". Inside a batch (beginBatch/endBatch) messages are
    // queued and sent together as one multi-element array "[<m1>,<m2>,...]"
    // at endBatch(). The website must always unwrap the top-level array.
    //
    // Messages are dropped (with a MLOG_WARN) when the queue exceeds
    // kMaxQueuedBatchMessages or when the WebSocket send buffer is full.
    // Callers must not rely on guaranteed delivery of non-critical updates.
    std::vector<String> messageQueue;
    bool batchingActive = false;

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
