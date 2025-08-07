/**
 * @file Config.h
 * @brief Configuration constants for the Marble Track project
 * 
 * This file contains all configuration constants including network credentials,
 * device settings, and system parameters.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

// Network Configuration
namespace Config {
    // WiFi Credentials
    // Replace with your network credentials
    const char* WIFI_SSID = "telenet-182FE_____";
    const char* WIFI_PASSWORD = "cPQdRWmFx1eM";
    
    // Access Point Configuration
    const char* AP_SSID = "MarbleTrack-Setup";
    const char* AP_PASSWORD = "marble123";
    const unsigned long WIFI_TIMEOUT_MS = 20000; // 20 seconds
    const unsigned long CONNECTION_CHECK_INTERVAL_MS = 500; // 0.5 seconds
    
    // Hardware Configuration
    const int SERVO_PWM_CHANNEL = 2;  // Changed from 7 to 2 to avoid buzzer conflicts
    
    // Device Pin Assignments
    const int LED_PIN = 1;
    const int SERVO_PIN = 21;
    const int BUTTON_PIN = 15;
    const int BUZZER_PIN = 14;
    
    // Button Configuration
    const unsigned long BUTTON_DEBOUNCE_MS = 50;
    const bool BUTTON_INVERTED = false;
    
    // Servo Configuration
    const int SERVO_INITIAL_ANGLE = 90;
    
    // System Configuration
    const unsigned long SERIAL_BAUD_RATE = 115200;
    const int WEB_SERVER_PORT = 80;
}

#endif
