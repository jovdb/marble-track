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

// Operation modes
enum class OperationMode
{
  MANUAL,
  AUTOMATIC
};

// Global mode variable
OperationMode currentMode = OperationMode::MANUAL;

// Timing variable for automatic mode
unsigned long lastAutoToggleTime = 0;

// Create network and server instances
Network network(Config::WIFI_SSID, Config::WIFI_PASSWORD);
AsyncWebServer server(Config::WEB_SERVER_PORT);
WebsiteHost websiteHost(&network);
WebSocketManager wsManager("/ws");
DeviceManager deviceManager;

// Device instances
Led testLed(Config::LED_PIN, "test-led", "Test LED");
ServoDevice testServo(Config::SERVO_PIN, "test-servo", "Test Servo", Config::SERVO_INITIAL_ANGLE, Config::SERVO_PWM_CHANNEL);
Button testButton(Config::BUTTON_PIN, "test-button", "Test Button", Config::BUTTON_INVERTED, Config::BUTTON_DEBOUNCE_MS);
Buzzer testBuzzer(Config::BUZZER_PIN, "test-buzzer", "Test Buzzer");

// Function declarations
void setOperationMode(OperationMode mode);

void runManualMode()
{
  // Manual mode: devices are controlled via WebSocket commands
  // Check for button press to toggle LED
  if (testButton.wasPressed())
  {
    testLed.toggle();
  }

  // This is the default mode where users control devices through the web interface
}

void runAutomaticMode()
{

  // Automatic mode: run predefined sequences or automation logic
  // Toggle LED every second
  unsigned long currentTime = millis();
  if (currentTime - lastAutoToggleTime >= 1000)
  {
    testLed.toggle();
    lastAutoToggleTime = currentTime;
  }

  if (currentTime % 10000 < 5)
  {
    testServo.setAngle(currentTime % 180); // Rotate servo every 10 seconds
  }
}

void setup()
{
  // Initialize serial communication
  Serial.begin(Config::SERIAL_BAUD_RATE);
  Serial.println("Starting Marble Track Communication System");

  // Initialize Network (will try WiFi, fall back to AP if needed)
  bool networkInitialized = network.initialize();
  if (!networkInitialized) {
    Serial.println("ERROR: Network initialization failed! System may not be accessible.");
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

  testServo.setup(); // Initialize servo hardware
  testServo.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                   { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testServo);

  testButton.setup(); // Initialize button hardware
  testButton.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testButton);

  testBuzzer.setup(); // Initialize buzzer hardware
  testBuzzer.setStateChangeCallback([&](const String &deviceId, const String &stateJson)
                                    { wsManager.broadcastState(deviceId, stateJson, ""); });
  deviceManager.addDevice(&testBuzzer);

  Serial.println("Device management:");
  Serial.println("  Total devices: " + String(deviceManager.getDeviceCount()));
  Serial.println("State change broadcasting enabled");

  // Initialize in MANUAL mode
  Serial.println("Operation mode: MANUAL");
  Serial.println("Use setOperationMode() to switch between MANUAL and AUTOMATIC");

  testServo.setAngle(20); // Set initial angle for servo
  
  // Print access information
  Serial.println("\n=== DEVICE READY ===");
  Serial.println("==================\n");
}

void loop()
{
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
    Serial.println("Operation mode changed to: " + String(modeString));

    // Optional: broadcast mode change via WebSocket
    // You could add WebSocket message broadcasting here if needed
  }
}
