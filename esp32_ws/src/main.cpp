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
#include "devices/DividerWheel.h"
#include "DeviceManager.h"
#include <devices/Buzzer.h>
#include "esp_log.h"

#include "devices/Stepper.h"
#include "devices/PwmMotor.h"

#include <devices/Wheel.h>
#include "OperationMode.h"
#include "SerialConsole.h"
#include "OtaUpload.h"
#include "AutoMode.h"
#include "ManualMode.h"

OperationMode currentMode = OperationMode::MANUAL;

// Timing variable for automatic mode
unsigned long lastAutoToggleTime = 0;

#include "NetworkSettings.h"

// Create network and server instances
Network *network = nullptr; // Will be created after loading settings
AsyncWebServer server(80);
LittleFSManager littleFSManager;
// WebsiteHost websiteHost(&network);
WebsiteHost *websiteHost = nullptr; // Will be created after network initialization
WebSocketManager wsManager(nullptr, nullptr, "/ws");

// Device instances
// ...existing code...

// Function declarations
void setOperationMode(OperationMode mode);
void globalNotifyClientsCallback(const String &message);

SerialConsole *serialConsole = nullptr;


void globalNotifyClientsCallback(const String &message)
{
  if (wsManager.hasClients()) {
    wsManager.notifyClients(message);
  }
}

DeviceManager deviceManager(globalNotifyClientsCallback);

AutoMode autoMode(deviceManager);
ManualMode manualMode(deviceManager);

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
  serialConsole = new SerialConsole(deviceManager, network, currentMode);

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

  deviceManager.setHasClients([]() { return wsManager.hasClients(); });

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // Setup  Devices with callback to enable state change notifications during initialization
  deviceManager.setup();

  // Setup AutoMode after all devices are initialized
  autoMode.setup();

  // Setup ManualMode after all devices are initialized
  manualMode.setup();

  // Set callback for device changes
  deviceManager.setOnDevicesChanged([]() {
    autoMode.setup();
    manualMode.setup();
  });

  // Startup sound
  Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  if (buzzer)
  {
    MLOG_INFO("Startup tone");
    buzzer->startupTone(); // Play startup tone sequence
  }

  MLOG_INFO("Device management initialized - Total devices: %d", deviceManager.getDeviceCount());

  // State change broadcasting is now enabled during setup

  // Initialize in MANUAL mode
  MLOG_INFO("Operation mode: MANUAL");

  MLOG_INFO("System initialization complete!");
}

void loop()
{
  // Check for serial input commands
  if (serialConsole)
  {
    serialConsole->loop();
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

  // State machine based on current mode
  switch (currentMode)
  {
  case OperationMode::MANUAL:
    manualMode.loop();
    break;

  case OperationMode::AUTOMATIC:
    autoMode.loop();
    break;
  }
}

void setOperationMode(OperationMode mode)
{
  if (currentMode != mode)
  {
    currentMode = mode;

    const char *modeString = (mode == OperationMode::MANUAL) ? "MANUAL" : "AUTOMATIC";

    MLOG_INFO("Operation mode changed to: %s", modeString);

    // Optional: broadcast mode change via WebSocket
    // You could add WebSocket message broadcasting here if needed
  }
}
