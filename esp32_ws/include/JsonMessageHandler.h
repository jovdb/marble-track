/**
 * @file JsonMessageHandler.h
 * @brief WebSocket JSON message handling for Marble Track Controller
 * 
 * This file contains the declarations for JSON message processing,
 * device status reporting, and command handling for the ESP32-based
 * marble track control system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef JSON_MESSAGE_HANDLER_H
#define JSON_MESSAGE_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief Initialize the JSON message handler
 * 
 * Sets up any required initialization for JSON message processing.
 * Should be called during system setup.
 */
void initializeJsonHandler();

/**
 * @brief Create a standardized JSON response
 * 
 * Generates a consistent JSON response format with success status,
 * message, optional data, and request correlation ID.
 * 
 * @param success Whether the operation was successful
 * @param message Response message describing the result
 * @param data Optional JSON data string to include in response
 * @param requestId Optional request ID for response correlation
 * @return Formatted JSON response string
 */
String createJsonResponse(bool success, const String& message, const String& data = "", const String& requestId = "");

/**
 * @brief Create device status JSON response
 * 
 * Generates a comprehensive status report including current device state,
 * hardware status, and connection information.
 * 
 * @return Device status as formatted JSON string
 */
String createDeviceStatusJson();

/**
 * @brief Create device information JSON response
 * 
 * Generates detailed device information including hardware specifications,
 * available commands, current state, and system metadata.
 * 
 * @return Device information as formatted JSON string
 */
String createDeviceInfoJson();

/**
 * @brief Process custom JSON commands
 * 
 * Parses and executes JSON commands received via WebSocket. Supports
 * hardware control, system information queries, and GPIO operations.
 * 
 * @param command The command object from parsed JSON
 * @param requestId Optional request ID for response correlation
 * @return JSON response string indicating command result
 */
String processCustomCommand(ArduinoJson::JsonObject command, const String& requestId = "");

/**
 * @brief Handle incoming WebSocket JSON message
 * 
 * Main entry point for processing WebSocket messages. Validates JSON format,
 * determines message type, and routes to appropriate handler functions.
 * 
 * @param message Raw message string received from WebSocket
 * @return JSON response string to send back to client
 */
String handleJsonMessage(const String& message);

/**
 * @brief Create discovery response JSON
 * 
 * Generates a device discovery response for clients scanning for
 * available marble track controllers on the network.
 * 
 * @return Discovery response as formatted JSON string
 */
String createDiscoveryResponse();

/**
 * @brief Validate GPIO pin number
 * 
 * Checks if the specified pin number is valid for GPIO operations
 * on the ESP32 platform.
 * 
 * @param pin Pin number to validate
 * @return true if pin is valid for GPIO operations, false otherwise
 */
bool isValidGpioPin(int pin);

#endif // JSON_MESSAGE_HANDLER_H
