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
// ...existing code...

// Function declarations
void setOperationMode(OperationMode mode);

void runManualMode()
{
  // Manual mode: devices are controlled via WebSocket commands
  // Check for button press to toggle LED
  Button *testButton = deviceManager.getDeviceByIdAs<Button>("test-button");
  Buzzer *testBuzzer = deviceManager.getDeviceByIdAs<Buzzer>("test-buzzer");
  Wheel *wheel = deviceManager.getDeviceByIdAs<Wheel>("wheel");
  Button *testButton2 = deviceManager.getDeviceByIdAs<Button>("test-button2");

  if (testButton && testBuzzer && wheel && testButton->isPressed()) {
    testBuzzer->tone(200, 100);
    wheel->move(8000); // Move stepper 100 steps on button press
  }

  // Check for second button press to trigger buzzer
  if (testButton2 && testBuzzer && testButton2->wasPressed()) {
    testBuzzer->tone(1000, 200); // Play 1000Hz tone for 200ms
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
  bool networkInitialized = network.setup();
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

  Led *testLed = new Led(1, "test-led", "Test LED");
  testLed->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                  { wsManager.broadcastState(deviceId, stateJson, ""); });
  testLed->setup();
  deviceManager.addDevice(testLed);

  Button *testButton = new Button(15, "test-button", "Test Button", false, 50);
  testButton->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                     { wsManager.broadcastState(deviceId, stateJson, ""); });
  testButton->setup();
  deviceManager.addDevice(testButton);

  Button *testButton2 = new Button(16, "test-button2", "Test Button 2", false, 50);
  testButton2->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                      { wsManager.broadcastState(deviceId, stateJson, ""); });
  testButton2->setup();
  deviceManager.addDevice(testButton2);

  Buzzer *testBuzzer = new Buzzer(14, "test-buzzer", "Test Buzzer");
  testBuzzer->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                     { wsManager.broadcastState(deviceId, stateJson, ""); });
  testBuzzer->setup();
  deviceManager.addDevice(testBuzzer);

  ServoDevice *testServo = new ServoDevice(8, "test-servo", "SG90", 0, 0);
  testServo->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  testServo->setup();
  deviceManager.addDevice(testServo);

  Button *ballSensor = new Button(47, "ball-sensor", "Ball Sensor", true, 100, Button::ButtonType::NormalClosed);
  ballSensor->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                     { wsManager.broadcastState(deviceId, stateJson, ""); });
  ballSensor->setup();
  deviceManager.addDevice(ballSensor);

  GateWithSensor *gateWithSensor = new GateWithSensor(21, 2, 48, testBuzzer, "gate-with-sensor", "Gate", 50, true, 50, Button::ButtonType::NormalClosed);
  gateWithSensor->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                         { wsManager.broadcastState(deviceId, stateJson, ""); });
  gateWithSensor->setup();
  deviceManager.addDevice(gateWithSensor);

  Stepper *stepper = new Stepper(45, 48, "stepper", "Stepper Motor", 1000, 500);
  stepper->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                  { wsManager.broadcastState(deviceId, stateJson, ""); });
  stepper->setup();
  deviceManager.addDevice(stepper);

  Stepper *testStepper = new Stepper(4, 5, 6, 7, "test-stepper", "28BYJ48", 1000, 500);
  testStepper->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                      { wsManager.broadcastState(deviceId, stateJson, ""); });
  testStepper->setup();
  deviceManager.addDevice(testStepper);

  Wheel *wheel = new Wheel(45, 48, 39, "wheel", "Wheel");
  wheel->setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                { wsManager.broadcastState(deviceId, stateJson, ""); });
  wheel->setup();
  deviceManager.addDevice(wheel);

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
