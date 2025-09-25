#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <algorithm>
#include <vector>
#include "Config.h"
#include "Logging.h"
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

#include "NetworkSettings.h"

// Create network and server instances
// Network network("telenet-182FE", "cPQdRWmFx1eM");
Network *network = nullptr; // Will be created after loading settings
AsyncWebServer server(80);
LittleFSManager littleFSManager;
// WebsiteHost websiteHost(&network);
WebsiteHost *websiteHost = nullptr; // Will be created after network initialization
DeviceManager deviceManager;
WebSocketManager wsManager(&deviceManager, nullptr, "/ws");

// Device instances
// ...existing code...

// Function declarations
void setOperationMode(OperationMode mode);
void logNetworkInfo();
void checkSerialCommands();

enum class SerialNetworkState
{
  Idle,
  SelectingNetwork,
  EnteringCustomSsid,
  EnteringPassword,
  Confirming
};

struct NetworkOption
{
  String ssid;
  int32_t rssi;
  wifi_auth_mode_t authMode;
};

struct SerialNetworkSession
{
  SerialNetworkState state = SerialNetworkState::Idle;
  std::vector<NetworkOption> networks;
  String stageBuffer;
  String selectedSsid;
  String password;
  int selectedIndex = -1;
  bool manualEntry = false;

  void reset()
  {
    state = SerialNetworkState::Idle;
    networks.clear();
    stageBuffer = "";
    selectedSsid = "";
    password = "";
    selectedIndex = -1;
    manualEntry = false;
  }
};

static SerialNetworkSession serialNetworkSession;

void startSetNetworkFlow();
void handleSetNetworkInput(char incoming);
void cancelSetNetworkFlow(const char *reason = nullptr);

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
    MLOG_INFO("Button test-button pressed - triggering buzzer and wheel");
    testBuzzer->tone(200, 100);
    wheel->move(8000); // Move stepper 100 steps on button press
  }

  // Check for second button press to trigger buzzer
  if (testButton2 && testBuzzer && testButton2->wasPressed())
  {
    MLOG_INFO("Button test-button2 pressed - playing tone (1000Hz, 200ms)");
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

static void showConfirmationPrompt()
{
  Serial.println();
  Serial.println("📦 Network configuration preview:");
  Serial.printf("  SSID: %s\n", serialNetworkSession.selectedSsid.c_str());
  if (serialNetworkSession.password.length() > 0)
  {
    String masked(serialNetworkSession.password.length(), '*');
    Serial.printf("  Password: %s\n", masked.c_str());
  }
  else
  {
    Serial.println("  Password: (open network)");
  }
  Serial.println();
  Serial.println("Press Enter to save these settings, or Esc to cancel.");
  serialNetworkSession.state = SerialNetworkState::Confirming;
}

void cancelSetNetworkFlow(const char *reason)
{
  Serial.println();
  if (reason && reason[0] != '\0')
  {
    Serial.printf("⚠️  %s\n", reason);
  }
  Serial.println("❎ Network configuration cancelled.");
  Serial.println();
  serialNetworkSession.reset();
}

void startSetNetworkFlow()
{
  serialNetworkSession.reset();

  Serial.println();
  Serial.println("🔍 Scanning for WiFi networks...");

  // Perform a synchronous scan (blocks briefly but keeps flow simple)
  int16_t networkCount = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);

  const size_t MAX_OPTIONS = 10;
  if (networkCount > 0)
  {
    serialNetworkSession.networks.reserve(std::min<int16_t>(networkCount, MAX_OPTIONS));
    for (int16_t idx = 0; idx < networkCount && serialNetworkSession.networks.size() < MAX_OPTIONS; ++idx)
    {
      NetworkOption option{WiFi.SSID(idx), WiFi.RSSI(idx), WiFi.encryptionType(idx)};

      if (option.ssid.isEmpty())
      {
        continue;
      }

      // Skip duplicate SSIDs to avoid clutter
      bool duplicate = false;
      for (const auto &existing : serialNetworkSession.networks)
      {
        if (existing.ssid == option.ssid)
        {
          duplicate = true;
          break;
        }
      }

      if (!duplicate)
      {
        serialNetworkSession.networks.push_back(option);
      }
    }

    std::sort(serialNetworkSession.networks.begin(), serialNetworkSession.networks.end(), [](const NetworkOption &a, const NetworkOption &b) {
      if (a.rssi == b.rssi)
      {
        return a.ssid < b.ssid;
      }
      return a.rssi > b.rssi;
    });
  }

  WiFi.scanDelete();

  if (serialNetworkSession.networks.empty())
  {
    Serial.println("⚠️  No WiFi networks detected.");
    Serial.println("Enter the SSID manually and press Enter (Esc to cancel).");
    Serial.println();
    Serial.print("SSID: ");
    serialNetworkSession.state = SerialNetworkState::EnteringCustomSsid;
    serialNetworkSession.manualEntry = true;
    serialNetworkSession.stageBuffer = "";
    return;
  }

  Serial.println();
  Serial.println("Available networks:");
  for (size_t optionIdx = 0; optionIdx < serialNetworkSession.networks.size(); ++optionIdx)
  {
    const auto &opt = serialNetworkSession.networks[optionIdx];
    Serial.printf("  %d. %s (%d dBm)%s\n",
                  static_cast<int>(optionIdx + 1),
                  opt.ssid.c_str(),
                  opt.rssi,
                  opt.authMode == WIFI_AUTH_OPEN ? " [open]" : "");
  }
  Serial.println("  0. Enter SSID manually");
  Serial.println();
  Serial.println("Type the number of the network and press Enter.");
  Serial.println("Press Esc at any time to cancel.");

  serialNetworkSession.state = SerialNetworkState::SelectingNetwork;
  serialNetworkSession.stageBuffer = "";
}

static void finishNetworkSelection(size_t selectedIndex)
{
  serialNetworkSession.manualEntry = false;
  serialNetworkSession.selectedIndex = static_cast<int>(selectedIndex);
  serialNetworkSession.selectedSsid = serialNetworkSession.networks[selectedIndex].ssid;

  Serial.printf("Selected network: %s\n", serialNetworkSession.selectedSsid.c_str());

  if (serialNetworkSession.networks[selectedIndex].authMode == WIFI_AUTH_OPEN)
  {
    Serial.println("This network is open. No password required.");
    serialNetworkSession.password = "";
    showConfirmationPrompt();
    return;
  }

  Serial.println("Enter the WiFi password (Esc to cancel).");
  Serial.print("Password: ");
  serialNetworkSession.stageBuffer = "";
  serialNetworkSession.password = "";
  serialNetworkSession.state = SerialNetworkState::EnteringPassword;
}

static void saveAndApplyNetworkSettings()
{
  if (serialNetworkSession.selectedSsid.isEmpty())
  {
    cancelSetNetworkFlow("SSID cannot be empty.");
    return;
  }

  NetworkSettings newSettings(serialNetworkSession.selectedSsid, serialNetworkSession.password);

  Serial.println();
  Serial.println("💾 Saving network settings...");

  bool saved = deviceManager.saveNetworkSettings(newSettings);
  if (!saved)
  {
    Serial.println("❌ Failed to write settings to /config.json.");
    serialNetworkSession.reset();
    Serial.println();
    return;
  }

  Serial.println("✅ Network credentials saved to /config.json.");

  if (network != nullptr)
  {
    NetworkMode result = network->applySettings(newSettings);
    switch (result)
    {
    case NetworkMode::WIFI_CLIENT:
      Serial.printf("✅ Connected to '%s' at %s\n",
                    WiFi.SSID().c_str(),
                    WiFi.localIP().toString().c_str());
      break;
    case NetworkMode::ACCESS_POINT:
      Serial.println("⚠️  Could not join the WiFi network. Fallback Access Point is active.");
      Serial.printf("     Connect to %s at %s to retry.\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
      break;
    case NetworkMode::DISCONNECTED:
    default:
      Serial.println("❌ Network connection failed. Check the credentials and try again.");
      break;
    }
  }
  else
  {
    Serial.println("⚠️  Network manager is not initialized yet. Settings will apply on next reboot.");
  }

  Serial.println();
  Serial.println("Use the 'network' command to check current status.");
  Serial.println();

  serialNetworkSession.reset();
}

void handleSetNetworkInput(char incoming)
{
  switch (serialNetworkSession.state)
  {
  case SerialNetworkState::SelectingNetwork:
  {
    if (incoming == 27)
    {
      cancelSetNetworkFlow(nullptr);
      return;
    }

    if (incoming == '\r')
    {
      return;
    }

    if (incoming == '\n')
    {
      Serial.println();
      if (serialNetworkSession.stageBuffer.isEmpty())
      {
        Serial.print("Select network #: ");
        return;
      }

      int choice = serialNetworkSession.stageBuffer.toInt();
      serialNetworkSession.stageBuffer = "";

      if (choice == 0)
      {
        Serial.println("Manual SSID entry selected.");
        Serial.println("Enter the SSID and press Enter (Esc to cancel).");
        Serial.print("SSID: ");
        serialNetworkSession.state = SerialNetworkState::EnteringCustomSsid;
        serialNetworkSession.manualEntry = true;
        return;
      }

      size_t index = (choice > 0) ? static_cast<size_t>(choice - 1) : SIZE_MAX;
      if (index < serialNetworkSession.networks.size())
      {
        finishNetworkSelection(index);
        return;
      }

      Serial.println("❌ Invalid selection. Try again.");
      Serial.print("Select network #: ");
      return;
    }

    if (incoming == 8 || incoming == 127)
    {
      if (!serialNetworkSession.stageBuffer.isEmpty())
      {
        serialNetworkSession.stageBuffer.remove(serialNetworkSession.stageBuffer.length() - 1);
        Serial.print("\b \b");
      }
      return;
    }

    if (isDigit(incoming))
    {
      serialNetworkSession.stageBuffer += incoming;
      Serial.print(incoming);
    }
    return;
  }
  case SerialNetworkState::EnteringCustomSsid:
  {
    if (incoming == 27)
    {
      cancelSetNetworkFlow(nullptr);
      return;
    }

    if (incoming == '\r')
    {
      return;
    }

    if (incoming == '\n')
    {
      if (serialNetworkSession.stageBuffer.isEmpty())
      {
        Serial.println();
        Serial.println("SSID cannot be empty. Try again.");
        Serial.print("SSID: ");
        return;
      }

      serialNetworkSession.selectedSsid = serialNetworkSession.stageBuffer;
      serialNetworkSession.stageBuffer = "";
      Serial.println();
      Serial.println("Enter the WiFi password (leave empty for open network, Esc to cancel).");
      Serial.print("Password: ");
      serialNetworkSession.password = "";
      serialNetworkSession.state = SerialNetworkState::EnteringPassword;
      return;
    }

    if (incoming == 8 || incoming == 127)
    {
      if (!serialNetworkSession.stageBuffer.isEmpty())
      {
        serialNetworkSession.stageBuffer.remove(serialNetworkSession.stageBuffer.length() - 1);
        Serial.print("\b \b");
      }
      return;
    }

    serialNetworkSession.stageBuffer += incoming;
    Serial.print(incoming);
    return;
  }
  case SerialNetworkState::EnteringPassword:
  {
    if (incoming == 27)
    {
      cancelSetNetworkFlow(nullptr);
      return;
    }

    if (incoming == '\r')
    {
      return;
    }

    if (incoming == '\n')
    {
      serialNetworkSession.password = serialNetworkSession.stageBuffer;
      serialNetworkSession.stageBuffer = "";
      Serial.println();
      showConfirmationPrompt();
      return;
    }

    if (incoming == 8 || incoming == 127)
    {
      if (!serialNetworkSession.stageBuffer.isEmpty())
      {
        serialNetworkSession.stageBuffer.remove(serialNetworkSession.stageBuffer.length() - 1);
        Serial.print("\b \b");
      }
      return;
    }

    serialNetworkSession.stageBuffer += incoming;
    Serial.print('*');
    return;
  }
  case SerialNetworkState::Confirming:
  {
    if (incoming == 27)
    {
      cancelSetNetworkFlow(nullptr);
      return;
    }

    if (incoming == '\r')
    {
      return;
    }

    if (incoming == '\n')
    {
      saveAndApplyNetworkSettings();
      return;
    }
    return;
  }
  case SerialNetworkState::Idle:
  default:
    break;
  }
}

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

  // First maount so config file can be loaded
  littleFSManager.setup();

  // Load network settings from configuration
  NetworkSettings networkSettings = deviceManager.loadNetworkSettings();

  // Use default settings if none found in config
  if (!networkSettings.isValid())
  {
    MLOG_INFO("No network settings found in config, using defaults");
    networkSettings = NetworkSettings("telenet-182FE", "cPQdRWmFx1eM");
  }

  // Create network instance with loaded settings
  network = new Network(networkSettings);

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
    otaService.setup(hostnameStr.c_str()); // <-- OTA setup only after network is ready
  }

  // Create WebsiteHost instance after network is initialized
  websiteHost = new WebsiteHost(network);

  // Initialize WebsiteHost with the network instance
  websiteHost->setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);
  wsManager.setNetwork(network);

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // // If no devices loaded, use hardcoded devices and save to disk
  // if (deviceManager.getDeviceCount() == 0)
  // {

  //   Led *led = new Led("test-led");
  //   led->setName("Test LED");

  //   Device *devices[] = {
  //       led,
  //       new Button("test-button2", "Test Button 2"),
  //       new Buzzer("test-buzzer", "Test Buzzer"),
  //       new DividerWheel(10, 11, 12, 13, 15, "wheel", "Divider Wheel")};

  //   const int numDevices = sizeof(devices) / sizeof(devices[0]);
  //   for (int i = 0; i < numDevices; ++i)
  //   {
  //     deviceManager.addDevice(devices[i]);
  //   }
  //   deviceManager.saveDevicesToJsonFile();
  // }
  /*
    // Add development/test devices (these are not saved to JSON)
    ServoDevice *testServo = new ServoDevice("test-servo", "SG90");
    testServo->setPin(42);
    testServo->setPwmChannel(1);
    deviceManager.addDevice(testServo);

    PwmMotor *testPwmMotor = new PwmMotor("test-pwm-motor", "Test PWM Motor");
    testPwmMotor->setupMotor(15, 0, 1000, 10); // pin 14, channel 0, 1kHz, 10-bit resolution
    deviceManager.addDevice(testPwmMotor);

    Stepper *testStepper = new Stepper("test-stepper", "Test Stepper");
    testStepper->configure2Pin(17, 18, 1000, 500);
    testStepper->setMaxSpeed(2000);
    // TODO: setMaxAcceleration(1000);
    deviceManager.addDevice(testStepper);
  */
  // Setup  Devices
  deviceManager.setup([&](const String &deviceId, const String &stateJson)
                      { wsManager.broadcastState(deviceId, stateJson, ""); });

  // Startup sound
  Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  if (buzzer)
  {
    MLOG_INFO("Startup tone");
    buzzer->startupTone(); // Play startup tone sequence
  }

  MLOG_INFO("Device management initialized - Total devices: %d", deviceManager.getDeviceCount());
  MLOG_INFO("State change broadcasting enabled");

  // Initialize in MANUAL mode
  MLOG_INFO("Operation mode: MANUAL");

  MLOG_INFO("System initialization complete!");
}

void loop()
{
  // Check for serial input commands
  checkSerialCommands();

  otaService.loop();

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
    if (network)
    {
      String hostname = network->getHostname();
      Serial.printf("  🏠 mDNS: http://%s.local\n", hostname.c_str());
    }
    else
    {
      Serial.println("  🏠 mDNS: http://marble-track.local");
    }
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
  static String commandBuffer;

  while (Serial.available() > 0)
  {
    char incoming = static_cast<char>(Serial.read());

    // If we're inside the interactive flow, delegate immediately
    if (serialNetworkSession.state != SerialNetworkState::Idle)
    {
      handleSetNetworkInput(incoming);
      continue;
    }

    if (incoming == '\r')
    {
      continue;
    }

    if (incoming == 27)
    {
      // Cancel current command input
      commandBuffer = "";
      Serial.println();
      Serial.println("❎ Command entry cancelled.");
      Serial.println();
      continue;
    }

    if (incoming == '\n')
    {
      String input = commandBuffer;
      commandBuffer = "";
      input.trim();

      Serial.println();

      if (input.length() == 0)
      {
        Serial.println("💡 Commands: 'devices', 'network', 'memory', 'restart'");
        Serial.println();
        continue;
      }

      if (input.equalsIgnoreCase("restart"))
      {
        Serial.println("🔄 Restarting ESP32...");
        delay(1000);
        ESP.restart();
      }
      else if (input.equalsIgnoreCase("devices"))
      {
        Serial.printf("⚙️  Devices: %d total | Mode: %s\n",
                      deviceManager.getDeviceCount(),
                      (currentMode == OperationMode::MANUAL) ? "MANUAL" : "AUTOMATIC");

        Device *deviceList[20];
        int deviceCount = 0;
        deviceManager.getDevices(deviceList, deviceCount, 20);

        if (deviceCount > 0)
        {
          for (int i = 0; i < deviceCount; i++)
          {
            if (deviceList[i] != nullptr)
            {
              Serial.printf("  %d. %s [%s] %s\n",
                            i + 1,
                            deviceList[i]->getType().c_str(),
                            deviceList[i]->getId().c_str(),
                            deviceList[i]->getName().c_str());

              String state = deviceList[i]->getState();
              Serial.printf("     State:  %s\n", state.c_str());

              String config = deviceList[i]->getConfig();
              Serial.printf("     Config: %s\n", config.c_str());

              Serial.println();
            }
          }
        }
        else
        {
          Serial.println("  No devices found");
        }
        Serial.println();
      }
      else if (input.equalsIgnoreCase("network"))
      {
        logNetworkInfo();
        Serial.println("  • Type 'set-network' to configure WiFi credentials via serial.");
        Serial.println();
      }
      else if (input.equalsIgnoreCase("memory"))
      {
        Serial.println("💾 Memory Status:");
        uint32_t free = ESP.getFreeHeap();
        uint32_t total = ESP.getHeapSize();
        uint32_t percent = (free * 100) / total;
        Serial.printf("   ⚡ Free: %d%% (%d bytes) | Total: %d bytes\n", percent, free, total);
        Serial.printf("   📈 Min Free: %d bytes | CPU: %d MHz\n", ESP.getMinFreeHeap(), ESP.getCpuFreqMHz());
        Serial.println();
      }
      else if (input.equalsIgnoreCase("set-network"))
      {
        startSetNetworkFlow();
      }
      else
      {
        Serial.printf("❓ Unknown command: '%s'\n", input.c_str());
        Serial.println();
      }
      continue;
    }

    if (incoming == 8 || incoming == 127)
    {
      if (!commandBuffer.isEmpty())
      {
        commandBuffer.remove(commandBuffer.length() - 1);
        Serial.print("\b \b");
      }
      continue;
    }

    // Accumulate printable characters
    commandBuffer += incoming;
    Serial.print(incoming);
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
