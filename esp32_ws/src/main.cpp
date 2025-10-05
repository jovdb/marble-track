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

#include <ArduinoOTA.h>
#include <Update.h>
#include <devices/Wheel.h>
#include "OperationMode.h"
#include "SerialConsole.h"

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

// Global callback function for state changes
void globalStateChangeCallback(const String &deviceId, const String &stateJson)
{
  wsManager.broadcastState(deviceId, stateJson, "");
}

// SerialConsole will be initialized after network is created
SerialConsole *serialConsole = nullptr;

namespace
{
  struct HttpOtaContext
  {
    bool authenticated = false;
    bool started = false;
    bool completed = false;
    bool success = false;
  };
}

static constexpr const char *OTA_HTTP_USER = "ota";
static constexpr const char *OTA_HTTP_PASS = "marbletrack";

void setupHttpOtaEndpoint(AsyncWebServer &server)
{
  server.on(
      "/ota",
      HTTP_POST,
      [](AsyncWebServerRequest *request)
      {
        HttpOtaContext *ctx = static_cast<HttpOtaContext *>(request->_tempObject);
        if (!ctx || !ctx->authenticated)
        {
          request->requestAuthentication();
          if (ctx)
          {
            delete ctx;
            request->_tempObject = nullptr;
          }
          return;
        }

        if (!ctx->completed)
        {
          ctx->success = false;
        }

        AsyncWebServerResponse *response = request->beginResponse(
            ctx->success ? 200 : 500,
            "text/plain",
            ctx->success ? "Update OK" : "Update failed");
        response->addHeader("Connection", "close");
        request->send(response);

        bool shouldRestart = ctx->success;
        delete ctx;
        request->_tempObject = nullptr;

        if (shouldRestart)
        {
          Serial.println("HTTP OTA update successful, rebooting");
          delay(100);
          ESP.restart();
        }
        else
        {
          Serial.println("HTTP OTA update failed");
        }
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        HttpOtaContext *ctx = static_cast<HttpOtaContext *>(request->_tempObject);
        if (!ctx)
        {
          ctx = new HttpOtaContext();
          ctx->authenticated = request->authenticate(OTA_HTTP_USER, OTA_HTTP_PASS);
          request->_tempObject = ctx;
        }

        if (!ctx->authenticated)
        {
          return;
        }

        if (index == 0)
        {
          Serial.printf("HTTP OTA upload start: %s\n", filename.c_str());
          ctx->started = Update.begin(UPDATE_SIZE_UNKNOWN);
          if (!ctx->started)
          {
            Update.printError(Serial);
          }
        }

        if (!ctx->started)
        {
          return;
        }

        if (len)
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }

        if (final)
        {
          ctx->completed = true;
          ctx->success = Update.end(true) && !Update.hasError();
          if (ctx->success)
          {
            Serial.println("HTTP OTA upload complete");
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });
}

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
    
    // Wait a bit for network to be fully established
    delay(1000);
    
    // Setup ArduinoOTA directly
    ArduinoOTA.setHostname(hostnameStr.c_str());
    ArduinoOTA.setPassword("marbletrack");
    ArduinoOTA.onStart([]() { Serial.println("OTA Update Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("OTA Update End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { 
      Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100))); 
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    MLOG_INFO("OTA service configured");
  }

  // Create WebsiteHost instance after network is initialized
  websiteHost = new WebsiteHost(network);

  // Initialize WebsiteHost with the network instance
  websiteHost->setup(server);

  // Setup WebSocket with message handler
  wsManager.setup(server);
  wsManager.setDeviceManager(&deviceManager);
  wsManager.setNetwork(network);

  // Expose HTTP-based OTA endpoint for PlatformIO uploads
  setupHttpOtaEndpoint(server);

  // Start ArduinoOTA before starting the web server
  ArduinoOTA.begin();
  Serial.println("ArduinoOTA service started");

  // Start server
  server.begin();

  // Try to load devices from JSON file
  deviceManager.loadDevicesFromJsonFile();

  // Setup  Devices - callback will be set after setup to avoid issues during initialization
  deviceManager.setup(nullptr);

  // Startup sound
  Buzzer *buzzer = deviceManager.getDeviceByTypeAs<Buzzer>("buzzer");
  if (buzzer)
  {
    MLOG_INFO("Startup tone");
    buzzer->startupTone(); // Play startup tone sequence
  }

  MLOG_INFO("Device management initialized - Total devices: %d", deviceManager.getDeviceCount());
  
  // Now set the state change callback after everything is initialized using global function
  Device *deviceList[20];
  int count;
  deviceManager.getDevices(deviceList, count, 20);
  for (int i = 0; i < count; i++)
  {
    if (deviceList[i])
    {
      deviceList[i]->setStateChangeCallback(globalStateChangeCallback);
    }
  }
  
  MLOG_INFO("State change broadcasting enabled");

  // Initialize in MANUAL mode
  MLOG_INFO("Operation mode: MANUAL");

  MLOG_INFO("System initialization complete!");
}

void loop()
{
  // Check for serial input commands
  if (serialConsole) {
    serialConsole->loop();
  }

  ArduinoOTA.handle();

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
