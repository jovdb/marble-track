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
#include "devices/DividerWheel.h"
#include "DeviceManager.h"
#include <devices/Buzzer.h>
#include "esp_log.h"

#include "devices/GateWithSensor.h"
#include "devices/Stepper.h"
#include "devices/PwmMotor.h"

#include "OTA_Support.h"
#include <devices/Wheel.h>

static const char *TAG = "main";

// Service instances
OTAService otaService;

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
LittleFSManager littleFSManager;
WebsiteHost websiteHost(&network);
DeviceManager deviceManager;
WebSocketManager wsManager(&deviceManager, "/ws");

// Device instances
// ...existing code...

// Function declarations
void setOperationMode(OperationMode mode);
void logNetworkInfo();
void checkSerialCommands();

void runManualMode()
{
  // Manual mode: devices are controlled via WebSocket commands
  // Check for button press to toggle LED
  Button *testButton = deviceManager.getDeviceByIdAs<Button>("test-button");
  Buzzer *testBuzzer = deviceManager.getDeviceByIdAs<Buzzer>("test-buzzer");
  Wheel *wheel = deviceManager.getDeviceByIdAs<Wheel>("wheel");
  Button *testButton2 = deviceManager.getDeviceByIdAs<Button>("test-button2");

  if (testButton && testBuzzer && wheel && testButton->isPressed())
  {
    testBuzzer->tone(200, 100);
    wheel->move(8000); // Move stepper 100 steps on button press
  }

  // Check for second button press to trigger buzzer
  if (testButton2 && testBuzzer && testButton2->wasPressed())
  {
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
  // Automatic mode: Future implementation for automated sequences
  // Currently not implemented - system runs in MANUAL mode
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
    String hostnameStr = network.getHostname();
    otaService.setup(hostnameStr.c_str()); // <-- OTA setup only after network is ready
  }

  littleFSManager.setup();

  // Initialize WebsiteHost with the network instance
  websiteHost.setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // If no devices loaded, use hardcoded devices and save to disk
  if (deviceManager.getDeviceCount() == 0)
  {
    Device *devices[] = {
        new Led("test-led", "Test LED"),
        new Button("test-button2", "Test Button 2"),
        new Buzzer("test-buzzer", "Test Buzzer"),
        new DividerWheel(10, 11, 12, 13, 15, "wheel", "Divider Wheel")};

    const int numDevices = sizeof(devices) / sizeof(devices[0]);
    for (int i = 0; i < numDevices; ++i)
    {
      deviceManager.addDevice(devices[i]);
    }
    deviceManager.saveDevicesToJsonFile();
  }

  // Add development/test devices (these are not saved to JSON)
  ServoDevice *testServo = new ServoDevice("test-servo", "SG90");
  testServo->setPin(42);
  testServo->setPwmChannel(1);
  deviceManager.addDevice(testServo);

  PwmMotor *testPwmMotor = new PwmMotor("test-pwm-motor", "Test PWM Motor");
  testPwmMotor->setupMotor(14, 0, 1000, 10); // pin 14, channel 0, 1kHz, 10-bit resolution
  deviceManager.addDevice(testPwmMotor);

  // Setup  Devices
  deviceManager.setup([&](const String &deviceId, const String &stateJson)
                      { wsManager.broadcastState(deviceId, stateJson, ""); });

  // Startup sound
  Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  if (buzzer)
    buzzer->startupTone(); // Play startup tone sequence

  ESP_LOGI(TAG, "Device management:");
  ESP_LOGI(TAG, "  Total devices: %d", deviceManager.getDeviceCount());
  ESP_LOGI(TAG, "State change broadcasting enabled");

  // Initialize in MANUAL mode
  ESP_LOGI(TAG, "Operation mode: MANUAL");
  ESP_LOGI(TAG, "Use setOperationMode() to switch between MANUAL and AUTOMATIC");
}

void loop()
{
  // Check for serial input commands
  checkSerialCommands();

  otaService.loop();

  littleFSManager.loop();

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

void logNetworkInfo()
{
  Serial.println();
  Serial.println("📡 Network Status:");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("  ✅ WiFi: %s\n", WiFi.SSID().c_str());
    Serial.printf("  🏠 mDNS: http://marble-track.local\n");
    Serial.printf("  🌍 Web: http://%s\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("  ❌ WiFi: Disconnected");
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
    {
      Serial.printf("  🏢 AP Mode: %s at %s\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    }
  }
  Serial.println();
}

void checkSerialCommands()
{
  // Check if there's any serial input available
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove whitespace and newline characters

    // Check for Enter key (empty input) or specific commands
    if (input.length() == 0)
    {
      // Enter key pressed - show network info
      logNetworkInfo();
      Serial.println("💡 Commands: 'restart', 'devices', 'network', 'memory'");
      Serial.println();
    }
    else if (input.equalsIgnoreCase("restart"))
    {
      Serial.println("🔄 Restarting ESP32...");
      delay(1000);
      ESP.restart();
    }
    else if (input.equalsIgnoreCase("devices"))
    {
      Serial.println();
      Serial.printf("⚙️  Devices: %d total | Mode: %s\n",
                    deviceManager.getDeviceCount(),
                    (currentMode == OperationMode::MANUAL) ? "MANUAL" : "AUTOMATIC");
      Serial.println();
    }
    else if (input.equalsIgnoreCase("network"))
    {
      logNetworkInfo();
    }
    else if (input.equalsIgnoreCase("memory"))
    {
      Serial.println();
      Serial.println("💾 Memory Status:");
      uint32_t free = ESP.getFreeHeap();
      uint32_t total = ESP.getHeapSize();
      uint32_t percent = (free * 100) / total;
      Serial.printf("   ⚡ Free: %d%% (%d bytes) | Total: %d bytes\n", percent, free, total);
      Serial.printf("   📈 Min Free: %d bytes | CPU: %d MHz\n", ESP.getMinFreeHeap(), ESP.getCpuFreqMHz());
      Serial.println();
    }
    else
    {
      Serial.printf("❓ Unknown command: '%s'\n", input.c_str());
      Serial.println();
    }
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
