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
namespace Config
{
    // WiFi Credentials
    // Replace with your network credentials
    inline const char *WIFI_SSID = "telenet-182FE";
    inline const char *WIFI_PASSWORD = "cPQdRWmFx1eM";
    inline const unsigned long WIFI_TIMEOUT_MS = 10000;             // 20 seconds
    inline const unsigned long CONNECTION_CHECK_INTERVAL_MS = 1000; // 0.5 seconds

    // Access Point Configuration
    inline const char *AP_SSID = "MarbleTrack";
    inline const char *AP_PASSWORD = "";

    // Hardware Configuration
    inline const int SERVO_PWM_CHANNEL = 2; // Changed from 7 to 2 to avoid buzzer conflicts

    // Device Pin Assignments
    inline const int LED_PIN = 1;
    inline const int SERVO_PIN = 21;
    inline const int BUTTON_PIN = 15;
    inline const int BUTTON2_PIN = 16;
    inline const int BALL_SENSOR_PIN = 47;
    inline const int BUZZER_PIN = 14;

    // Button Configuration
    inline const unsigned long BUTTON_DEBOUNCE_MS = 50;
    inline const bool BUTTON_INVERTED = false;

    // Servo Configuration
    inline const int SERVO_INITIAL_ANGLE = 90;

    // System Configuration
    inline const unsigned long SERIAL_BAUD_RATE = 115200;
    inline const int WEB_SERVER_PORT = 80;
}

#endif
