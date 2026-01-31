#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins/Pins.h"
#include "Config.h"
#include "Logging.h"
#include "Network.h"
#include "WebsiteHost.h"
#include "WebSocketManager.h"
#include "DeviceManager.h"
#include "devices/Buzzer.h"
#include "esp_log.h"

#include "SerialConsole.h"
#include "OtaUpload.h"
#include "AutoMode.h"
#include "devices/MarbleController.h"

// Composition-based devices
#include "devices/Led.h"
#include "devices/Button.h"
#include "devices/Stepper.h"
#include "devices/Servo.h"
#include "devices/mixins/ControllableMixin.h"

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

// Global status LED

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
devices::MarbleController *marbleController = nullptr;

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

  // Load logging settings from configuration
  deviceManager.loadLoggingSettings();

  // Load network settings from configuration
  NetworkSettings networkSettings = deviceManager.loadNetworkSettings();

  // Create network instance with loaded settings
  network = new Network(networkSettings);

  // Now create SerialConsole after network is initialized
  serialConsole = new SerialConsole(deviceManager, network, &wsManager);

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

  // Set notifyClients callback for ControllableMixin (for composition devices)
  // This enables automatic WebSocket notifications when device state changes
  ControllableMixin<Device>::setNotifyClients(globalNotifyClientsCallback);

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // Setup  Devices with callback to enable state change notifications during initialization
  deviceManager.setup();

  // Setup pin factory to resolve expander addresses
  PinFactory::setup();

  // Set callback for device changes
  deviceManager.setOnDevicesChanged([]()
                                    {
                                      // Cleanup existing modes
                                      if (autoMode)
                                      {
                                        delete autoMode;
                                        autoMode = nullptr;
                                      }
                                      if (marbleController)
                                      {
                                        delete marbleController;
                                        marbleController = nullptr;
                                      }

                                      // Recreate mode based on button state
                                      // TODO: Re-enable when Button is converted to composition device
                                      // const Button *manualBtn = deviceManager.getDeviceByIdAs<Button>("manual-btn");
                                      // if (manualBtn && manualBtn->isPressed())
                                      // {
                                      //   MLOG_INFO("Device changed: Initializing MANUAL mode");
                                      //   marbleController = deviceManager.getDeviceByIdAs<MarbleController>("marble-controller");
                                      //   marbleController->setup();
                                      // }
                                      // else
                                      // {
                                      // MLOG_INFO("Device changed: Initializing AUTOMATIC mode");
                                      // autoMode = new AutoMode(deviceManager);
                                      // autoMode->setup();

                                      // }
                                    });

  // TODO: Re-enable when Button is converted to composition device
  // const Button *manualBtn = deviceManager.getDeviceByIdAs<Button>("manual-btn");
  // if (manualBtn && manualBtn->isPressed())
  // {
  //   MLOG_INFO("Operation mode: MANUAL");
  //   marbleController = deviceManager.getDeviceByIdAs<MarbleController>("marble-controller");
  //   marbleController->setup();
  // }
  // else
  // {
  // MLOG_INFO("Operation mode: AUTOMATIC");
  // autoMode = new AutoMode(deviceManager);
  // autoMode->setup();

  marbleController = deviceManager.getDeviceByIdAs<devices::MarbleController>("marble-controller");
  // }

  // Startup sound
  // TODO: Re-enable when Buzzer is converted to composition device
  // Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  // if (buzzer)
  // {
  //   MLOG_INFO("Startup tone");
  //   buzzer->startupTone(); // Play startup tone sequence
  // }

  // MLOG_INFO("Device management initialized - Total devices: %d", deviceManager.getDeviceCount());

  // State change broadcasting is now enabled during setup

  MLOG_INFO("System initialization complete!");
  MLOG_INFO("--------------------------");
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

  OtaUpload::loop();

  littleFSManager.loop();

  // Process captive portal for access point mode
  if (network)
  {
    network->loop(); // Handle non-blocking network connection
    network->processCaptivePortal();
  }

  // Keep the WebSocket alive
  wsManager.loop();

  // Run all devices using DeviceManager
  deviceManager.loop();

  // Run the active mode
  if (marbleController)
  {
    // Since it's a device, loop is called via deviceManager.loop()
  }
  if (autoMode)
  {
    autoMode->loop();
  }

  // Send all batched WebSocket messages at once
  wsManager.endBatch();
}
