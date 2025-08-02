#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class TimeManager {
public:
    // Initialize NTP time synchronization
    static void initialize();
    
    // Get current Unix timestamp in milliseconds (like JavaScript Date.now())
    static unsigned long long getCurrentTimestamp();
    
    // Check if time is synchronized
    static bool isTimeSynchronized();
    
private:
    static bool timeSynced;
    static const char* ntpServer1;
    static const char* ntpServer2;
    static const long gmtOffset_sec;
    static const int daylightOffset_sec;
};

#endif // TIMEMANAGER_H
