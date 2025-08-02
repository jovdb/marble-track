/**
 * @file HardwareController.h
 * @brief Hardware control interface for Marble Track Controller
 * 
 * This file contains the declarations for hardware control functions
 * including motor direction, speed control, LED management, and GPIO
 * operations for the ESP32-based marble track system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>

// Global state variables declarations
extern String currentDirection;
extern int currentSpeed;
extern bool currentLedState;
extern int connectedClients;

/**
 * @brief Initialize hardware components
 * 
 * Sets up GPIO pins, initializes hardware state, and configures
 * motor control and LED outputs.
 */
void initializeHardware();

/**
 * @brief Set motor direction
 * 
 * Controls the motor direction using GPIO pins. Supports clockwise (CW),
 * counter-clockwise (CCW), and stop (STOP) commands.
 * 
 * @param value Direction command ("CW", "CCW", or "STOP")
 */
void setDirection(const String& value);

/**
 * @brief Set LED state
 * 
 * Controls the built-in LED or connected LED via GPIO pin.
 * 
 * @param state LED state (true = ON, false = OFF)
 */
void setLedState(bool state);

/**
 * @brief Set motor speed
 * 
 * Controls motor speed using PWM output. Speed is specified as
 * a percentage from 0-100%.
 * 
 * @param speed Speed percentage (0-100)
 */
void setSpeed(int speed);

/**
 * @brief Get current hardware state
 * 
 * Returns a summary of the current hardware state including
 * direction, speed, and LED status.
 * 
 * @return Hardware state summary string
 */
String getHardwareStatus();

/**
 * @brief Reset hardware to default state
 * 
 * Resets all hardware components to their default/safe state:
 * - Direction: STOP
 * - Speed: 0
 * - LED: OFF
 */
void resetHardware();

/**
 * @brief Update connected clients count
 * 
 * Updates the global count of connected WebSocket clients.
 * 
 * @param count Number of connected clients
 */
void updateConnectedClients(int count);

#endif // HARDWARE_CONTROLLER_H
