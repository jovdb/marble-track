#include "OTA_Support.h"
#include <ArduinoOTA.h>
#include <Arduino.h>

OTAService::OTAService() {}

void OTAService::setup(const char *hostname)
{
    if (hostname)
    {
        _hostname = hostname;
    }
    else
    {
        _hostname = "esp32-ota";
    }
    // Try without setting hostname to see if that helps
    // ArduinoOTA.setHostname(_hostname);
    
    // Add authentication for security (optional but recommended)
    ArduinoOTA.setPassword("marbletrack");
    
    ArduinoOTA.onStart([]()
                       { Serial.println("OTA Update Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("OTA Update End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
            Serial.printf("OTA Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
    
    // ArduinoOTA.begin() will be called in main.cpp after all initialization
    Serial.printf("OTA service configured (hostname: %s)\n", _hostname);
    Serial.println("OTA service configuration complete");
}

void OTAService::loop()
{
    ArduinoOTA.handle();
}
