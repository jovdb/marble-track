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
#include "Led.h"
#include "IDevice.h"
#include "IControllable.h"
#include "DeviceManager.h"

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

// Device management system
DeviceManager deviceManager;

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Starting Marble Track JSON Communication System");

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

  // Instantiate and initialize LED controller
  testLed = new Led(1, "test-led", "Test LED");
  testLed->setup();

  // Add device to management system
  deviceManager.addDevice(testLed);
  Serial.println("Device management:");
  Serial.println("  Total devices: " + String(deviceManager.getDeviceCount()));
  Serial.println("  Controllable devices: " + String(deviceManager.getControllableCount()));
}

void loop()
{
  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices using DeviceManager
  deviceManager.loop();

  // Small delay to prevent watchdog issues
  delay(10);
}
