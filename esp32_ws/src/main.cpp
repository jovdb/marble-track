/**
 * @file main.cpp
 * @brief Main application file for Marble Track Controller ESP32
 *
 * This file contains the main application logic, WebSocket event handling,
 * and system initialization for the ESP32-based marble track control system.
 * JSON message processing and hardware control are handled by separate modules.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"
#include "JsonMessageHandler.h"
#include "HardwareController.h"
#include "Led.h"
#include "IDevice.h"
#include "IControllable.h"

// Replace with your network credentials
// const char *ssid = "REPLACE_WITH_YOUR_SSID";
// const char *password = "REPLACE_WITH_YOUR_PASSWORD";
const char *ssid = "telenet-182FE";
const char *password = "cPQdRWmFx1eM";

// Create server and instances
AsyncWebServer server(80);
WebsiteHost websiteHost(ssid, password);
WebSocketManager wsManager("/ws");
Led *testLed = nullptr; // Will be instantiated in setup()

// Array of all devices
const int MAX_DEVICES = 20;
IDevice *devices[MAX_DEVICES];
int devicesCount = 0;

// Array of controllable devices
const int MAX_CONTROLLABLES = 10;
IControllable *controllables[MAX_CONTROLLABLES];
int controllablesCount = 0;

// Helper function to add a device
void addDevice(IDevice *device)
{
  if (devicesCount < MAX_DEVICES && device != nullptr)
  {
    // Add to main device list
    devices[devicesCount] = device;
    devicesCount++;
    Serial.println("Added device: " + device->getId() + " (" + device->getName() + ")");
  }
}

// Helper function to add controllable device
void addControllable(IControllable *device)
{
  if (controllablesCount < MAX_CONTROLLABLES && device != nullptr)
  {
    controllables[controllablesCount] = device;
    controllablesCount++;
    Serial.println("Added controllable device: " + device->getId());
  }
}

// Helper function to find controllable device by ID
IControllable *findControllableById(const String &deviceId)
{
  for (int i = 0; i < controllablesCount; i++)
  {
    if (controllables[i]->getId() == deviceId)
    {
      return controllables[i];
    }
  }
  return nullptr;
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Starting Marble Track JSON Communication System");

  // Initialize hardware components
  initializeHardware();

  // Instantiate and initialize LED controller
  testLed = new Led(1, "test-led", "Test LED");
  testLed->setup();

  // Add device to management arrays
  addDevice(testLed);
  addControllable(testLed);
  Serial.println("Device arrays populated:");
  Serial.println("  Total devices: " + String(devicesCount));
  Serial.println("  Controllable devices: " + String(controllablesCount));

  // Initialize JSON message handler
  initializeJsonHandler();

  // Initialize TimeManager
  TimeManager::initialize();

  // Initialize WebsiteHost
  websiteHost.setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);

  // Start server
  server.begin();
  Serial.println("Marble Track JSON Communication System initialized");
  Serial.println("JSON Commands available:");
  Serial.println("- Discovery: {\"type\": \"discovery\"}");
  Serial.println("- Control: {\"action\": \"set_direction\", \"value\": \"CW\"}");
  Serial.println("- GPIO: {\"action\": \"set_gpio\", \"pin\": 2, \"state\": true}");
  Serial.println("- Info: {\"action\": \"get_info\"}");
  Serial.println("Connect via WebSocket to receive full command examples");

  // Print current hardware status
  Serial.println(getHardwareStatus());
}

void loop()
{
  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices (they all have loop() function now)
  for (int i = 0; i < devicesCount; i++)
  {
    if (devices[i] != nullptr)
    {
      devices[i]->loop();
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}
