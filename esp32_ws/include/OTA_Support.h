
#ifndef OTA_SUPPORT_H
#define OTA_SUPPORT_H

#include <ArduinoOTA.h>
#include <Arduino.h>

class OTAService {
public:
    OTAService(const char* hostname = "marble-track") : _hostname(hostname) {}

    void setup() {
        ArduinoOTA.setHostname(_hostname);
        ArduinoOTA.onStart([]() {
            Serial.println("OTA Update Start");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("OTA Update End");
        });
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
        ArduinoOTA.begin();
    }

    void loop() {
        ArduinoOTA.handle();
    }

private:
    const char* _hostname;
};

#endif // OTA_SUPPORT_H
