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
#include "devices/MarbleController.h"
#include "SongConstants.h"

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
bool otaConfigured = false;

void globalNotifyClientsCallback(const String &message)
{
  if (wsManager.hasClients())
  {
    wsManager.notifyClients(message);
  }
}

DeviceManager deviceManager(globalNotifyClientsCallback);

devices::MarbleController *marbleController = nullptr;

void setup()
{
  // Initialize serial communication
  Serial.begin(Config::SERIAL_BAUD_RATE);

  // Logging setup
  esp_log_level_set("*", ESP_LOG_INFO);
  // esp_log_level_set("main", ESP_LOG_DEBUG);
  // esp_log_level_set("Network", ESP_LOG_DEBUG);

  // Using simplified logging macros
  MLOG_INFO("Starting Marble Track System");
  MLOG_INFO("Build version: %s %s", __DATE__, __TIME__);

  // Fetch the reset reason
  esp_reset_reason_t reason = esp_reset_reason();

  switch (reason)
  {
  case ESP_RST_POWERON:
    // Serial.println("Power-on reset");
    break;
  case ESP_RST_EXT:
    MLOG_INFO("Restarted by external pin...");
    break;
  case ESP_RST_SW:
    MLOG_INFO("Restarted by software (esp_restart)...");
    break;
  case ESP_RST_PANIC:
    MLOG_ERROR("Restarted due to software crash / exception! (ESP_RST_PANIC)");
    break;
  case ESP_RST_INT_WDT:
    MLOG_ERROR("Restarted due to interrupt watchdog reset! (ESP_RST_INT_WDT)");
    break;
  case ESP_RST_TASK_WDT:
    MLOG_ERROR("Restarted due to task watchdog reset! (ESP_RST_TASK_WDT)");
    break;
  case ESP_RST_DEEPSLEEP:
    MLOG_INFO("Wakeup from Deep Sleep");
    break;
  case ESP_RST_BROWNOUT:
    MLOG_ERROR("Brownout reset (Voltage dip)! (ESP_RST_BROWNOUT)");
    break;
  case ESP_RST_SDIO:
    MLOG_INFO("Restarted over SDIO");
    break;
  default:
    MLOG_WARN("Unknown reset reason");
  }

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

  // Set callback for device changes.
  // NOTE: `marbleController` is a cached pointer to a Device that is owned by
  // `deviceManager` (set in loop() via getDeviceByIdAs<MarbleController>("controller")).
  // Deleting it here would leave a dangling pointer inside DeviceManager.devices[],
  // which crashes on the next deviceManager.loop() iteration and causes the ESP
  // to restart after every device-save-config (and can lose subsequent state
  // updates such as wheel breakpoints). Just clear our cached pointer; the
  // device-tree teardown/setup is already handled by handleDeviceSaveConfig.
  deviceManager.setOnDevicesChanged([]()
                                    {
                                      marbleController = nullptr;
                                    });

  MLOG_INFO("System initialization complete!");
  MLOG_INFO("--------------------------");
}

void loop()
{
  static unsigned long loopCount = 0;
  static unsigned long lastLoopRateLogMs = 0;

  if (lastLoopRateLogMs == 0)
  {
    lastLoopRateLogMs = millis();
  }

  loopCount++;

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

    if (!otaConfigured && network->getMode() != NetworkMode::DISCONNECTED)
    {
      OtaUpload::setup(*network, server);
      otaConfigured = true;
    }

    marbleController = deviceManager.getDeviceByIdAs<devices::MarbleController>("controller");
    static auto hasLoggedNoNetwork = false;
    if (!network->isModeChanged() && !network->isWiFiConnected() && marbleController)
    {
      if (!hasLoggedNoNetwork && network && !network->isConnecting() && network->getMode() != NetworkMode::WIFI_CLIENT && marbleController)
      {
        hasLoggedNoNetwork = true;
        MLOG_WARN("Not connected to WiFi at startup - queueing NO_NETWORK sound");
        marbleController->getAudio()->play(songs::NO_NETWORK, devices::Hv20tPlayMode::QueueIfPlaying);
      }
    }
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

  const unsigned long now = millis();
  const unsigned long elapsedMs = now - lastLoopRateLogMs;
  if (elapsedMs >= 1000)
  {
    const float loopsPerSecond = (static_cast<float>(loopCount) * 1000.0f) / static_cast<float>(elapsedMs);
    MLOG_LOOPS("%.0f loops/sec", loopsPerSecond, loopCount, elapsedMs);
    loopCount = 0;
    lastLoopRateLogMs = now;
  }

  // Send all batched WebSocket messages at once
  wsManager.endBatch();
}
