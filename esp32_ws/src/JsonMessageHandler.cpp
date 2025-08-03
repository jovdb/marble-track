/**
 * @file JsonMessageHandler.cpp
 * @brief WebSocket JSON message handling implementation for Marble Track Controller
 * 
 * This file implements the JSON message processing functionality including
 * command parsing, device status reporting, and response generation for
 * the ESP32-based marble track control system.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#include "JsonMessageHandler.h"
#include "TimeManager.h"
#include <WiFi.h>
#include <ESP.h>

// TODO: Update JsonMessageHandler to use DeviceManager instead of global variables
// Placeholder variables for now
String currentDirection = "STOP";
int currentSpeed = 0;
bool currentLedState = false;
int connectedClients = 0;

void initializeJsonHandler() {
  Serial.println("JSON Message Handler initialized");
}

String createJsonResponse(bool success, const String& message, const String& data, const String& requestId) {
  JsonDocument response;
  response["success"] = success;
  response["message"] = message;
  response["timestamp"] = TimeManager::getCurrentTimestamp();
  
  if (requestId.length() > 0) {
    response["requestId"] = requestId;
  }
  
  if (data.length() > 0) {
    JsonDocument dataDoc;
    deserializeJson(dataDoc, data);
    response["data"] = dataDoc;
  }
  
  String jsonString;
  serializeJson(response, jsonString);
  return jsonString;
}

String createDeviceStatusJson() {
  JsonDocument status;
  status["type"] = "device_status";
  status["deviceId"] = "marble_track_001";
  status["timestamp"] = TimeManager::getCurrentTimestamp();
  
  JsonObject state = status["state"].to<JsonObject>();
  state["direction"] = currentDirection;
  state["speed"] = currentSpeed;
  state["ledState"] = currentLedState;
  state["connectedClients"] = connectedClients;
  state["connectionStatus"] = (connectedClients > 0) ? "connected" : "disconnected";
  
  String jsonString;
  serializeJson(status, jsonString);
  return jsonString;
}

String createDeviceInfoJson() {
  JsonDocument deviceInfo;
  deviceInfo["type"] = "device_info";
  deviceInfo["deviceId"] = "marble_track_001";
  deviceInfo["deviceName"] = "Marble Track Controller";
  deviceInfo["deviceType"] = "motor_controller";
  deviceInfo["version"] = "1.0.0";
  deviceInfo["timestamp"] = TimeManager::getCurrentTimestamp();
  
  // Current state
  JsonObject state = deviceInfo["state"].to<JsonObject>();
  state["direction"] = currentDirection;
  state["speed"] = currentSpeed;
  state["ledState"] = currentLedState;
  state["connectedClients"] = connectedClients;
  
  // Available commands
  JsonArray commands = deviceInfo["availableCommands"].to<JsonArray>();
  commands.add("set_direction");
  commands.add("set_speed");
  commands.add("set_led");
  commands.add("get_status");
  commands.add("get_device_info");
  commands.add("get_info");
  commands.add("set_gpio");
  commands.add("read_gpio");
  commands.add("ping");
  commands.add("restart");
  
  String jsonString;
  serializeJson(deviceInfo, jsonString);
  return jsonString;
}

String createDiscoveryResponse() {
  JsonDocument discovery;
  discovery["type"] = "discovery_response";
  discovery["deviceId"] = "marble_track_001";
  discovery["deviceName"] = "Marble Track Controller";
  discovery["deviceType"] = "motor_controller";
  discovery["version"] = "1.0.0";
  discovery["ipAddress"] = WiFi.localIP().toString();
  discovery["macAddress"] = WiFi.macAddress();
  discovery["timestamp"] = TimeManager::getCurrentTimestamp();
  
  // Capabilities
  JsonArray capabilities = discovery["capabilities"].to<JsonArray>();
  capabilities.add("direction_control");
  capabilities.add("speed_control");
  capabilities.add("led_control");
  capabilities.add("gpio_control");
  capabilities.add("status_monitoring");
  
  String jsonString;
  serializeJson(discovery, jsonString);
  return jsonString;
}

bool isValidGpioPin(int pin) {
  // ESP32-S3 valid GPIO pins (excluding restricted pins)
  // Avoid pins: 0 (boot), 19-20 (USB), 26-32 (SPI flash), 33-37 (SPI flash on some variants)
  if (pin < 0 || pin > 48) return false;
  
  // Restricted pins to avoid
  if (pin >= 19 && pin <= 20) return false; // USB pins
  if (pin >= 26 && pin <= 32) return false; // SPI flash pins
  if (pin >= 33 && pin <= 37) return false; // Additional SPI flash pins on some variants
  
  return true;
}

String processCustomCommand(JsonObject command, const String& requestId) {
  String action = command["action"].as<String>();
  
  if (action == "ping") {
    return createJsonResponse(true, "pong", "", requestId);
  }
  else if (action == "get_status") {
    return createDeviceStatusJson();
  }
  else if (action == "get_device_info") {
    return createDeviceInfoJson();
  }
  else if (action == "set_direction") {
    String direction = command["value"] | "STOP";
    if (direction == "CW" || direction == "CCW" || direction == "STOP") {
      // TODO: Use DeviceManager to control motor device
      currentDirection = direction;
      return createJsonResponse(true, "Direction set to " + direction + " (placeholder)", "", requestId);
    } else {
      return createJsonResponse(false, "Invalid direction. Use CW, CCW, or STOP", "", requestId);
    }
  }
  else if (action == "set_speed") {
    int speed = command["value"] | 0;
    if (speed >= 0 && speed <= 100) {
      // TODO: Use DeviceManager to control motor device
      currentSpeed = speed;
      return createJsonResponse(true, "Speed set to " + String(speed) + "% (placeholder)", "", requestId);
    } else {
      return createJsonResponse(false, "Invalid speed. Use 0-100", "", requestId);
    }
  }
  else if (action == "set_led") {
    bool state = command["value"] | false;
    // TODO: Use DeviceManager to control LED device
    currentLedState = state;
    return createJsonResponse(true, "LED set to " + String(state ? "ON" : "OFF") + " (placeholder)", "", requestId);
  }
  else if (action == "restart") {
    // Schedule restart after sending response
    return createJsonResponse(true, "Device will restart in 2 seconds", "", requestId);
  }
  else if (action == "get_info") {
    JsonDocument info;
    info["deviceName"] = "Marble Track Controller";
    info["version"] = "1.0.0";
    info["chipModel"] = ESP.getChipModel();
    info["freeHeap"] = ESP.getFreeHeap();
    info["uptime"] = millis();
    info["wifiRSSI"] = WiFi.RSSI();
    info["ipAddress"] = WiFi.localIP().toString();
    info["macAddress"] = WiFi.macAddress();
    
    String infoString;
    serializeJson(info, infoString);
    return createJsonResponse(true, "Device information", infoString, requestId);
  }
  else if (action == "set_gpio") {
    int pin = command["pin"] | -1;
    bool state = command["state"] | false;
    
    if (isValidGpioPin(pin)) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, state ? HIGH : LOW);
      
      JsonDocument data;
      data["pin"] = pin;
      data["state"] = state;
      String dataString;
      serializeJson(data, dataString);
      
      return createJsonResponse(true, "GPIO pin " + String(pin) + " set to " + String(state ? "HIGH" : "LOW"), dataString, requestId);
    } else {
      return createJsonResponse(false, "Invalid pin number. Use valid GPIO pins (1-18, 21-25, 38-48)", "", requestId);
    }
  }
  else if (action == "read_gpio") {
    int pin = command["pin"] | -1;
    
    if (isValidGpioPin(pin)) {
      pinMode(pin, INPUT);
      bool state = digitalRead(pin);
      
      JsonDocument data;
      data["pin"] = pin;
      data["state"] = state;
      String dataString;
      serializeJson(data, dataString);
      
      return createJsonResponse(true, "GPIO pin " + String(pin) + " read as " + String(state ? "HIGH" : "LOW"), dataString, requestId);
    } else {
      return createJsonResponse(false, "Invalid pin number. Use valid GPIO pins (1-18, 21-25, 38-48)", "", requestId);
    }
  }
  else {
    return createJsonResponse(false, "Unknown command: " + action + ". Use ping, get_status, get_device_info, set_direction, set_speed, set_led, set_gpio, read_gpio, get_info, or restart", "", requestId);
  }
}

String handleJsonMessage(const String& message) {
  Serial.println("JSON Handler: Processing message: " + message);
  
  // Try to parse as JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("JSON parse error: " + String(error.c_str()));
    return createJsonResponse(false, "Invalid JSON format: " + String(error.c_str()));
  }

  // Check message type and action
  String messageType = doc["type"] | "";
  String action = doc["action"] | "";
  String requestId = doc["requestId"] | "";
  
  Serial.println("Message type: " + messageType + ", Action: " + action);
  
  // Handle discovery messages
  if (messageType == "discovery") {
    Serial.println("Processing discovery request");
    return createDiscoveryResponse();
  }
  
  // Handle command messages
  if (action.length() > 0) {
    JsonObject command = doc.as<JsonObject>();
    return processCustomCommand(command, requestId);
  }
  
  // Handle direct JSON object commands (legacy support)
  if (doc.is<JsonObject>() && doc.size() > 0) {
    JsonObject command = doc.as<JsonObject>();
    return processCustomCommand(command, requestId);
  }
  
  return createJsonResponse(false, "Invalid message format. Expected 'type' or 'action' field", "", requestId);
}
