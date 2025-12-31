#include "OtaUpload.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>

#include "Logging.h"
#include "Network.h"

namespace {
struct HttpOtaContext {
  bool authenticated = false;
  bool started = false;
  bool completed = false;
  bool success = false;
};

constexpr const char *OTA_HTTP_USER = "ota";
constexpr const char *OTA_HTTP_PASS = "marbletrack";

bool gConfigured = false;

void setupHttpOtaEndpoint(AsyncWebServer &server) {
  server.on(
      "/ota",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        auto *ctx = static_cast<HttpOtaContext *>(request->_tempObject);
        if (!ctx || !ctx->authenticated) {
          request->requestAuthentication();
          if (ctx) {
            delete ctx;
            request->_tempObject = nullptr;
          }
          return;
        }

        if (!ctx->completed) {
          ctx->success = false;
        }

        AsyncWebServerResponse *response = request->beginResponse(
            ctx->success ? 200 : 500,
            "text/plain",
            ctx->success ? "Update OK" : "Update failed");
        response->addHeader("Connection", "close");
        request->send(response);

        const bool shouldRestart = ctx->success;
        delete ctx;
        request->_tempObject = nullptr;

        if (shouldRestart) {
          MLOG_INFO("HTTP OTA update successful, rebooting");
          delay(100);
          ESP.restart();
        } else {
          MLOG_ERROR("HTTP OTA update failed");
        }
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        auto *ctx = static_cast<HttpOtaContext *>(request->_tempObject);
        if (!ctx) {
          ctx = new HttpOtaContext();
          ctx->authenticated = request->authenticate(OTA_HTTP_USER, OTA_HTTP_PASS);
          request->_tempObject = ctx;
        }

        if (!ctx->authenticated) {
          return;
        }

        if (index == 0) {
          MLOG_INFO("HTTP OTA upload start: %s", filename.c_str());
          ctx->started = Update.begin(UPDATE_SIZE_UNKNOWN);
          if (!ctx->started) {
            Update.printError(Serial);
          }
        }

        if (!ctx->started) {
          return;
        }

        if (len) {
          if (Update.write(data, len) != len) {
            Update.printError(Serial);
          }
        }

        if (final) {
          ctx->completed = true;
          ctx->success = Update.end(true) && !Update.hasError();
          if (ctx->success) {
            MLOG_INFO("HTTP OTA upload complete");
          } else {
            Update.printError(Serial);
          }
        }
      });
}
} // namespace

namespace OtaUpload {

void setup(Network &network, AsyncWebServer &server) {
  attemptSetup(network, server);
}

void attemptSetup(Network &network, AsyncWebServer &server) {
  if (gConfigured) {
    return;
  }

  // Only setup OTA if network is connected (WiFi client or AP mode)
  if (network.getCurrentMode() == NetworkMode::DISCONNECTED) {
    return; // Silently return - will retry in loop()
  }

  const String hostname = network.getHostname();
  MLOG_INFO("Configuring OTA services with hostname: %s", hostname.c_str());

  ArduinoOTA.setHostname(hostname.c_str());
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

  setupHttpOtaEndpoint(server);

  ArduinoOTA.begin();
  Serial.println("ArduinoOTA service started");

  gConfigured = true;
}

void loop(Network &network, AsyncWebServer &server) {
  if (!gConfigured) {
    // Try to setup OTA if not configured yet and network becomes available
    attemptSetup(network, server);
    return;
  }

  ArduinoOTA.handle();
}

} // namespace OtaUpload
