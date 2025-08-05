#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"
#include "devices/Led.h"
#include "devices/Servo.h"
#include "devices/Device.h"
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
DeviceManager deviceManager;
Led testLed(1, "test-led", "Test LED");                    // Global LED instance
ServoDevice testServo(21, "test-servo", "Test Servo", 90); // Global Servo instance

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Starting Marble Track JSON Communication System");

  // Initialize TimeManager
  TimeManager::initialize();

  // Initialize WebsiteHost
  websiteHost.setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);

  // Start server
  server.begin();

  // Setup devices
  testServo.setup(); // Initialize servo hardware

  // Add device to management system
  testLed.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                 { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testLed);

  testServo.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                   { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testServo);

  Serial.println("Device management:");
  Serial.println("  Total devices: " + String(deviceManager.getDeviceCount()));
  Serial.println("  Controllable devices: " + String(deviceManager.getControllableCount()));
  Serial.println("State change broadcasting enabled");

  testServo.setAngle(20); // Set initial angle for servo
}

void loop()
{
  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices using DeviceManager
  deviceManager.loop();
}
