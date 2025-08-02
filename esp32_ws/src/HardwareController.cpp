/**
 * @file HardwareController.cpp
 * @brief Hardware control implementation for Marble Track Controller
 * 
 * This file implements the hardware control functionality including
 * motor direction control, speed management, LED operations, and GPIO
 * handling for the ESP32-based marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "HardwareController.h"

// GPIO pin definitions
#define DIRECTION_PIN 1    // GPIO pin for motor direction control
#define LED_PIN 2         // GPIO pin for LED control
#define SPEED_PWM_PIN 3   // GPIO pin for PWM speed control (if used)

// Global state variables
String currentDirection = "STOP";
int currentSpeed = 0;
bool currentLedState = false;
int connectedClients = 0;

void initializeHardware() {
  Serial.println("Initializing hardware components...");
  
  // Initialize GPIO pins
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SPEED_PWM_PIN, OUTPUT);
  
  // Set initial states
  digitalWrite(DIRECTION_PIN, LOW);    // Motor stopped
  digitalWrite(LED_PIN, LOW);          // LED off
  digitalWrite(SPEED_PWM_PIN, LOW);    // Speed at 0
  
  // Initialize global state
  currentDirection = "STOP";
  currentSpeed = 0;
  currentLedState = false;
  connectedClients = 0;
  
  Serial.println("Hardware initialization complete");
  Serial.println("- Direction Pin: " + String(DIRECTION_PIN));
  Serial.println("- LED Pin: " + String(LED_PIN));
  Serial.println("- Speed PWM Pin: " + String(SPEED_PWM_PIN));
}

void setDirection(const String& value) {
  Serial.println("Hardware: Direction changed to: " + value);
  currentDirection = value;

  if (value == "CW") {
    digitalWrite(DIRECTION_PIN, HIGH);
    Serial.println("GPIO " + String(DIRECTION_PIN) + ": HIGH (CW)");
  } else if (value == "CCW") {
    digitalWrite(DIRECTION_PIN, LOW);
    Serial.println("GPIO " + String(DIRECTION_PIN) + ": LOW (CCW)");
  } else if (value == "STOP") {
    digitalWrite(DIRECTION_PIN, LOW);
    // Also stop speed when stopping
    setSpeed(0);
    Serial.println("GPIO " + String(DIRECTION_PIN) + ": LOW (STOP)");
  } else {
    Serial.println("Warning: Invalid direction value: " + value);
  }
}

void setLedState(bool state) {
  Serial.println("Hardware: LED state changed to: " + String(state ? "ON" : "OFF"));
  currentLedState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  Serial.println("GPIO " + String(LED_PIN) + ": " + String(state ? "HIGH" : "LOW"));
}

void setSpeed(int speed) {
  // Constrain speed to valid range
  speed = constrain(speed, 0, 100);
  
  Serial.println("Hardware: Speed changed to: " + String(speed) + "%");
  currentSpeed = speed;
  
  // Convert percentage to PWM value (0-255)
  int pwmValue = map(speed, 0, 100, 0, 255);
  
  // For ESP32, use analogWrite or ledcWrite for PWM
  analogWrite(SPEED_PWM_PIN, pwmValue);
  
  Serial.println("PWM output on pin " + String(SPEED_PWM_PIN) + ": " + String(pwmValue) + "/255");
  
  // If speed is 0, ensure direction is also stopped
  if (speed == 0 && currentDirection != "STOP") {
    Serial.println("Speed set to 0, automatically stopping motor");
    setDirection("STOP");
  }
}

String getHardwareStatus() {
  String status = "Hardware Status:\n";
  status += "- Direction: " + currentDirection + "\n";
  status += "- Speed: " + String(currentSpeed) + "%\n";
  status += "- LED: " + String(currentLedState ? "ON" : "OFF") + "\n";
  status += "- Connected Clients: " + String(connectedClients) + "\n";
  status += "- Direction Pin (" + String(DIRECTION_PIN) + "): " + String(digitalRead(DIRECTION_PIN) ? "HIGH" : "LOW") + "\n";
  status += "- LED Pin (" + String(LED_PIN) + "): " + String(digitalRead(LED_PIN) ? "HIGH" : "LOW");
  return status;
}

void resetHardware() {
  Serial.println("Resetting hardware to default state...");
  
  setDirection("STOP");
  setSpeed(0);
  setLedState(false);
  
  Serial.println("Hardware reset complete");
}

void updateConnectedClients(int count) {
  connectedClients = count;
  Serial.println("Connected clients updated: " + String(count));
  
  // Optional: Turn on LED when clients are connected
  if (count > 0 && !currentLedState) {
    Serial.println("Clients connected - LED indicator enabled");
    // Uncomment the line below if you want LED to auto-turn on when clients connect
    // setLedState(true);
  }
}
