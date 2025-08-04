#include "TimeManager.h"

// Static member initialization
bool TimeManager::timeSynced = false;
const char *TimeManager::ntpServer1 = "pool.ntp.org";
const char *TimeManager::ntpServer2 = "time.nist.gov";
const long TimeManager::gmtOffset_sec = 0;
const int TimeManager::daylightOffset_sec = 0;

void TimeManager::initialize()
{
    Serial.print("Time synchronization..");

    // Configure NTP time synchronization
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    // Wait for time to be set
    time_t now = 0;
    int attempts = 0;
    const int maxAttempts = 30;

    while (now < 8 * 3600 * 2 && attempts < maxAttempts)
    {
        delay(250);
        time(&now);
        Serial.print(".");
        attempts++;
    }

    if (now >= 8 * 3600 * 2)
    {
        timeSynced = true;
        Serial.println(": OK!");

        /*
        // Print current time for verification
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        */
    }
    else
    {
        Serial.println(": ERROR");
        Serial.println("Continuing without NTP sync - timestamps will be relative");
    }
}

unsigned long long TimeManager::getCurrentTimestamp()
{
    if (timeSynced)
    {
        time_t now;
        time(&now);
        return (unsigned long long)now * 1000ULL; // Convert seconds to milliseconds
    }
    else
    {
        // Fallback to millis() if time sync failed
        Serial.println("Warning: Using millis() fallback - time not synchronized");
        return (unsigned long long)millis();
    }
}

bool TimeManager::isTimeSynchronized()
{
    return timeSynced;
}
