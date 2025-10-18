#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward declaration
class DeviceManager;
#include "Network.h"

class WebSocketManager
{
private:
    AsyncWebSocket ws;
    DeviceManager *deviceManager;
    Network *network;
    bool scanInProgress = false;

    // Helper methods for cleaner message handling
    void handleRestart();
    void handleDeviceFunction(JsonDocument &doc);
    void handleDeviceState(JsonDocument &doc);
    void handleDeviceGetState(JsonDocument &doc);
    void handleGetDevices(JsonDocument &doc);

public:
    WebSocketManager(DeviceManager *deviceManager, Network *network, const char *path = "/ws");
    void setup(AsyncWebServer &server);
    void loop();
    void notifyClients(String state);
    String getStatus() const;
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void setDeviceManager(DeviceManager *deviceManager);
    void setNetwork(Network *network);

    // State change broadcasting
    void broadcastState(const String &deviceId, const String &stateJson, const String &error);

    // Made public to allow global function access
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

    // Device config handlers
    void handleDeviceSaveConfig(JsonDocument &doc);
    void handleDeviceReadConfig(JsonDocument &doc);
    void handleSetDevicesConfig(JsonDocument &doc);
    void handleGetDevicesConfig(JsonDocument &doc);
    
    // Device management handlers
    void handleAddDevice(JsonDocument &doc);
    void handleRemoveDevice(JsonDocument &doc);
    void handleSetDeviceOrder(JsonDocument &doc);
    
    // Network config handlers
    void handleGetNetworkConfig(JsonDocument &doc);
    void handleSetNetworkConfig(JsonDocument &doc);
    void handleGetNetworks(JsonDocument &doc);
    void handleGetNetworkStatus(JsonDocument &doc);
};

#endif
