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
#include "IControllable.h"
#include "ILoopable.h"

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

// Array of controllable devices
const int MAX_CONTROLLABLES = 10;
IControllable *controllables[MAX_CONTROLLABLES];
int controllablesCount = 0;

// Array of loopable devices
const int MAX_LOOPABLES = 10;
ILoopable *loopables[MAX_LOOPABLES];
int loopablesCount = 0;

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

// Helper function to add loopable device
void addLoopable(ILoopable *device)
{
  if (loopablesCount < MAX_LOOPABLES && device != nullptr)
  {
    loopables[loopablesCount] = device;
    loopablesCount++;
    Serial.println("Added loopable device to loop array");
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

  // Populate controlables array
  addControllable(testLed);
  addLoopable(testLed);
  Serial.println("Controllables array populated with " + String(controllablesCount) + " devices");
  Serial.println("Loopables array populated with " + String(loopablesCount) + " devices");

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

  // Run all loopable devices
  for (int i = 0; i < loopablesCount; i++)
  {
    if (loopables[i] != nullptr)
    {
      loopables[i]->loop();
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}
