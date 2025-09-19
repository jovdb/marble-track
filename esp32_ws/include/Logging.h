/**
 * @file Logging.h
 * @brief Simple logging system for Marble Track project
 *
 * Provides simplified logging macros without colors for cleaner output.
 * All macros can be disabled by setting MARBLE_LOG_ENABLED to 0.
 *
 * Available macros:
 * - MLOG_INFO(format, ...)     - General information messages
 * - MLOG_ERROR(format, ...)    - Error messages
 * - MLOG_WARN(format, ...)     - Warning messages
 * - MLOG_WS_SEND(format, ...)  - WebSocket send messages
 * - MLOG_WS_RECEIVE(format, ...) - WebSocket receive messages
 *
 * Usage examples:
 *   MLOG_INFO("System started");
 *   MLOG_ERROR("Failed to initialize device: %s", deviceId);
 *   MLOG_WS_SEND("Message sent to client #%u", clientId);
 *
 * All messages include timestamp in milliseconds since boot: [12345]
 * To disable all logging, set MARBLE_LOG_ENABLED to 0 below.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

// Enable/disable logging (set to 0 to disable all logging)
#define MARBLE_LOG_ENABLED 1

// Simple logging macros without colors
#if MARBLE_LOG_ENABLED

#define MLOG_INFO(format, ...) \
    Serial.printf("[%lu] INFO: " format "\n", millis(), ##__VA_ARGS__)

#define MLOG_ERROR(format, ...) \
    Serial.printf("[%lu] ERROR: " format "\n", millis(), ##__VA_ARGS__)

#define MLOG_WARN(format, ...) \
    Serial.printf("[%lu] WARN: " format "\n", millis(), ##__VA_ARGS__)

#define MLOG_WS_SEND(format, ...) \
    Serial.printf("[%lu] WS_SEND: " format "\n", millis(), ##__VA_ARGS__)

#define MLOG_WS_RECEIVE(format, ...) \
    Serial.printf("[%lu] WS_RECV: " format "\n", millis(), ##__VA_ARGS__)

#else
// Logging disabled - all macros become no-ops (zero overhead)
#define MLOG_INFO(format, ...)
#define MLOG_ERROR(format, ...)
#define MLOG_WARN(format, ...)
#define MLOG_WS_SEND(format, ...)
#define MLOG_WS_RECEIVE(format, ...)
#endif

#endif // LOGGING_H