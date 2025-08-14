#include "OTA_Support.h"
OTAService otaService;
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Network.h"
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"
#include "devices/Led.h"
#include "devices/Servo.h"
#include "devices/Button.h"
#include "devices/Device.h"
#include "DeviceManager.h"
#include <devices/Buzzer.h>
#include "esp_log.h"

#include "devices/GateWithSensor.h"
#include "devices/Stepper.h"

#include "OTA_Support.h"
#include <devices/Wheel.h>

static const char *TAG = "main";

enum class OperationMode
{
  MANUAL,
  AUTOMATIC
};

OperationMode currentMode = OperationMode::MANUAL;

// Timing variable for automatic mode
unsigned long lastAutoToggleTime = 0;

// Create network and server instances
Network network("telenet-182FE", "cPQdRWmFx1eM");
AsyncWebServer server(80);
WebsiteHost websiteHost(&network);
WebSocketManager wsManager("/ws");
DeviceManager deviceManager;

// Device instances
Led testLed(1, "test-led", "Test LED");
Button testButton(15, "test-button", "Test Button", false, 50);
Button testButton2(16, "test-button2", "Test Button 2", false, 50);
Buzzer testBuzzer(14, "test-buzzer", "Test Buzzer");
Button ballSensor(47, "ball-sensor", "Ball Sensor", true, 100, Button::ButtonType::NormalClosed);
Stepper stepper(45, 48, "stepper", "Stepper Motor", 1000, 500);

GateWithSensor gateWithSensor(21, 2, 48, &testBuzzer, "gate-with-sensor", "Gate", 50, true, 50, Button::ButtonType::NormalClosed);
Button ballInGate(48, "ball-in-gate", "Ball In Gate", true, 100, Button::ButtonType::NormalClosed);
Wheel wheel(45, 48, 39, "wheel", "Wheel");

// Function declarations
void setOperationMode(OperationMode mode);

void runManualMode()
{
  // Manual mode: devices are controlled via WebSocket commands
  // Check for button press to toggle LED
  if (testButton.isPressed())
  {
    testBuzzer.tone(200, 100);
    wheel.move(8000); // Move stepper 100 steps on button press
  }

  // Check for second button press to trigger buzzer
  if (testButton2.wasPressed())
  {
    testBuzzer.tone(1000, 200); // Play 1000Hz tone for 200ms
  }

  if (ballSensor.wasPressed())
  {
    testBuzzer.tone(400, 200);
  }

  /*
  static unsigned long ballActionStart = 0;
  static int ballActionStep = 0;
  if (ballSensor.wasPressed())
  {
    Serial.println("Ball detected!");
    testLed.set(true);
    ballActionStart = millis();
    ballActionStep = 1;
  }
  // Non-blocking sequence for ball sensor actions
  if (ballActionStep == 1 && millis() - ballActionStart >= 350)
  {
    // testServo.setSpeed(80);
    testServo.setSpeed(240);
    testServo.setAngle(170);
    testBuzzer.tone(400, 200);
    ballActionStart = millis();
    ballActionStep = 2;
  }
  if (ballActionStep == 2 && millis() - ballActionStart >= 750)
  {
    // testServo.setSpeed(60);
    testServo.setSpeed(200);
    testServo.setAngle(28);
    ballActionStep = 0;
  }
  else if (ballSensor.wasReleased())
  {
    // Ball sensor released, perform action
    Serial.println("Ball released!");
    testLed.set(false); // Turn off LED
  }
  */

  // This is the default mode where users control devices through the web interface
}

void runAutomaticMode()
{
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Logging setup
  esp_log_level_set("*", ESP_LOG_INFO);
  // esp_log_level_set("main", ESP_LOG_DEBUG);
  // esp_log_level_set("Network", ESP_LOG_DEBUG);

  ESP_LOGI(TAG, "Starting Marble Track");

  // Initialize Network (will try WiFi, fall back to AP if needed)
  bool networkInitialized = network.initialize();
  if (!networkInitialized)
  {
    ESP_LOGE(TAG, "ERROR: Network initialization failed! System may not be accessible.");
  }
  else
  {
    otaService.setup(); // <-- OTA setup only after network is ready
  }

  // Initialize WebsiteHost with the network instance
  websiteHost.setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);

  // Start server
  server.begin();

  // Setup devices
  testLed.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                 { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testLed);

  testButton.setup(); // Initialize button hardware
  testButton.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testButton);

  testButton2.setup(); // Initialize second button hardware
  testButton2.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                     { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testButton2);

  testBuzzer.setup(); // Initialize buzzer hardware
  testBuzzer.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testBuzzer);

  ballSensor.setup(); // Initialize ball sensor hardware
  ballSensor.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&ballSensor);

  gateWithSensor.setup(); // Initialize GateWithSensor device
  gateWithSensor.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                        { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&gateWithSensor);

  stepper.setup(); // Initialize Stepper device
  stepper.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                 { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&stepper);

  wheel.setup(); // Initialize Wheel device
  wheel.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                               { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&wheel);

  ESP_LOGI(TAG, "Device management:");
  ESP_LOGI(TAG, "  Total devices: %d", deviceManager.getDeviceCount());
  ESP_LOGI(TAG, "State change broadcasting enabled");

  // Initialize in MANUAL mode
  ESP_LOGI(TAG, "Operation mode: MANUAL");
  ESP_LOGI(TAG, "Use setOperationMode() to switch between MANUAL and AUTOMATIC");
}

void loop()
{
  otaService.loop();
  // Process captive portal for access point mode
  network.processCaptivePortal();

  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices using DeviceManager
  deviceManager.loop();

  // State machine based on current mode
  switch (currentMode)
  {
  case OperationMode::MANUAL:
    runManualMode();
    break;

  case OperationMode::AUTOMATIC:
    runAutomaticMode();
    break;
  }
}

void setOperationMode(OperationMode mode)
{
  if (currentMode != mode)
  {
    currentMode = mode;

    const char *modeString = (mode == OperationMode::MANUAL) ? "MANUAL" : "AUTOMATIC";
    ESP_LOGI(TAG, "Operation mode changed to: %s", modeString);

    // Optional: broadcast mode change via WebSocket
    // You could add WebSocket message broadcasting here if needed
  }
}
