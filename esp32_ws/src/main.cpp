#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Logging.h"
#include "Network.h"
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "TimeManager.h"
#include "devices/Led.h"
#include "devices/Button.h"
#include "devices/Device.h"
#include "DeviceManager.h"
#include <devices/Buzzer.h>
#include "esp_log.h"

#include "devices/Stepper.h"
#include "devices/PwmMotor.h"
#include "devices/Servo.h"

#include <devices/Wheel.h>
#include "SerialConsole.h"
#include "OtaUpload.h"
#include "AutoMode.h"
#include "ManualMode.h"

// Composition-based LED test
#include "devices/composition/Led.h"

// Timing variable for automatic mode
unsigned long lastAutoToggleTime = 0;

#include "NetworkSettings.h"
#include <devices/LedDevice.h>
#include <devices/ButtonDevice.h>
#include <devices/TestTaskDevice.h>

// Create network and server instances
Network *network = nullptr; // Will be created after loading settings
AsyncWebServer server(80);
LittleFSManager littleFSManager;
// WebsiteHost websiteHost(&network);
WebsiteHost *websiteHost = nullptr; // Will be created after network initialization
WebSocketManager wsManager(nullptr, nullptr, "/ws");

// Global status LED
LedDevice *statusLed = nullptr;

// Button press toggle for LED blinking
bool blinkingActive = false;
bool lastButtonPressed = false;

// Device instances
// ...existing code...

// Function declarations
void globalNotifyClientsCallback(const String &message);

SerialConsole *serialConsole = nullptr;

void globalNotifyClientsCallback(const String &message)
{
  if (wsManager.hasClients())
  {
    wsManager.notifyClients(message);
  }
}

DeviceManager deviceManager(globalNotifyClientsCallback);

AutoMode *autoMode = nullptr;
ManualMode *manualMode = nullptr;

// Test LED using composition pattern
composition::Led *testLed2 = nullptr;

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Logging setup
  esp_log_level_set("*", ESP_LOG_INFO);
  // esp_log_level_set("main", ESP_LOG_DEBUG);
  // esp_log_level_set("Network", ESP_LOG_DEBUG);

  // Using simplified logging macros
  MLOG_INFO("Starting Marble Track System");
  MLOG_INFO("Build version: %s %s", __DATE__, __TIME__);

  // First mount so config file can be loaded
  littleFSManager.setup();

  // Load network settings from configuration
  NetworkSettings networkSettings = deviceManager.loadNetworkSettings();

  // Create network instance with loaded settings
  network = new Network(networkSettings);

  // Now create SerialConsole after network is initialized
  serialConsole = new SerialConsole(deviceManager, network, autoMode, manualMode);

  // Initialize Network (will try WiFi, fall back to AP if needed)
  bool networkInitialized = network->setup();

  if (!networkInitialized)
  {
    MLOG_ERROR("Network initialization failed! System may not be accessible.");
  }
  else
  {
    String hostnameStr = network->getHostname();
    MLOG_INFO("Network ready, hostname: %s.local", hostnameStr.c_str());

    OtaUpload::setup(*network, server);
  }

  // Create WebsiteHost instance after network is initialized
  websiteHost = new WebsiteHost(network);

  // Initialize WebsiteHost with the network instance
  websiteHost->setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);
  wsManager.setNetwork(network);

  // Set callback to check for connected clients (don't broadcast WS messages if no clients are connected)
  // This must be set BEFORE loading devices so that child devices get the callback during construction
  deviceManager.setHasClients([]()
                              { return wsManager.hasClients(); });

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // Setup  Devices with callback to enable state change notifications during initialization
  deviceManager.setup();

  // Set callback for device changes
  deviceManager.setOnDevicesChanged([]()
                                    {
    // Cleanup existing modes
    if (autoMode) {
      delete autoMode;
      autoMode = nullptr;
    }
    if (manualMode) {
      delete manualMode;
      manualMode = nullptr;
    }
    
    // Recreate mode based on button state
    const Button *manualBtn = deviceManager.getDeviceByIdAs<Button>("manual-btn");
    if (manualBtn && manualBtn->isPressed())
    {
      MLOG_INFO("Device changed: Initializing MANUAL mode");
      manualMode = new ManualMode(deviceManager);
      manualMode->setup();
    }
    else
    {
      MLOG_INFO("Device changed: Initializing AUTOMATIC mode");
      autoMode = new AutoMode(deviceManager);
      autoMode->setup();
    } });

  const Button *manualBtn = deviceManager.getDeviceByIdAs<Button>("manual-btn");

  if (manualBtn && manualBtn->isPressed())
  {
    MLOG_INFO("Operation mode: MANUAL");
    manualMode = new ManualMode(deviceManager);
    manualMode->setup();
  }
  else
  {
    MLOG_INFO("Operation mode: AUTOMATIC");
    autoMode = new AutoMode(deviceManager);
    autoMode->setup();
  }

  // Startup sound
  Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  if (buzzer)
  {
    MLOG_INFO("Startup tone");
    buzzer->startupTone(); // Play startup tone sequence
  }

  // MLOG_INFO("Device management initialized - Total devices: %d", deviceManager.getDeviceCount());

  // State change broadcasting is now enabled during setup

  // Create TestTaskDevice with its own button and LED children
  TestTaskDevice *testTask = new TestTaskDevice("test-task", globalNotifyClientsCallback);
  testTask->setup();
  deviceManager.addTaskDevice(testTask);

  // Create test Servo on pin 16
  Servo *testServo = new Servo("test-servo", globalNotifyClientsCallback);
  JsonDocument servoConfig;
  servoConfig["name"] = "Test Servo";
  servoConfig["pin"] = 16;
  servoConfig["mcpwmChannel"] = -1;   // Auto-acquire
  servoConfig["frequency"] = 50;      // Standard servo frequency
  servoConfig["minDutyCycle"] = 2.5;  // 0 degrees
  servoConfig["maxDutyCycle"] = 12.5; // 180 degrees
  servoConfig["defaultDurationInMs"] = 500;
  testServo->setup(servoConfig);
  deviceManager.addTaskDevice(testServo);

  // Create composition LED test on pin 15
  testLed2 = new composition::Led("led2");

  // Configure pin before setup
  composition::LedConfig ledConfig;
  ledConfig.pin = 15;
  testLed2->setConfig(ledConfig);
  deviceManager.addDevice(testLed2);
  testLed2->setup();
  testLed2->onStateChange([](void *data)
                          {
    if (data)
    {
      composition::LedState* state = static_cast<composition::LedState*>(data);
      MLOG_INFO("Composition LED state changed: mode=%s, onTime=%lu, offTime=%lu",
                state->mode.c_str(), state->blinkOnTime, state->blinkOffTime);
    } });

  testLed2->blink(); // Turn on at startup
  MLOG_INFO("Composition LED test created on pin 15");

  MLOG_INFO("System initialization complete!");
}

void loop()
{
  // Begin batching WebSocket messages for this loop iteration
  wsManager.beginBatch();

  // Check for serial input commands
  if (serialConsole)
  {
    serialConsole->loop();
  }

  // Update composition LED test
  if (testLed2)
  {
    testLed2->loop();
  }

  OtaUpload::loop();

  littleFSManager.loop();

  // Process captive portal for access point mode
  if (network)
  {
    network->processCaptivePortal();
  }

  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices using DeviceManager
  deviceManager.loop();

  // Run the active mode
  if (manualMode)
  {
    manualMode->loop();
  }
  if (autoMode)
  {
    autoMode->loop();
  }

  // Send all batched WebSocket messages at once
  wsManager.endBatch();
}
