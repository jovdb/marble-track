/**
 * @file Logging.h
 * @brief Conditional logging system for Marble Track project
 *
 * Provides logging macros with runtime enable/disable per log type.
 *
 * Available macros:
 * - MLOG_INFO(format, ...)     - General information messages
 * - MLOG_ERROR(format, ...)    - Error messages
 * - MLOG_WARN(format, ...)     - Warning messages
 * - MLOG_DEBUG(format, ...)    - Debug messages
 * - MLOG_WS_SEND(format, ...)  - WebSocket send messages
 * - MLOG_WS_RECEIVE(format, ...) - WebSocket receive messages
 *
 * Usage examples:
 *   MLOG_INFO("System started");
 *   MLOG_ERROR("Failed to initialize device: %s", deviceId);
 *   MLOG_WS_SEND("Message sent to client #%u", clientId);
 *
 * All messages include timestamp in milliseconds since boot and the current task name: [12345][I][loop]
 * Use the 'logging' serial command to enable/disable log types at runtime.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Enable/disable logging (set to 0 to disable all logging)
#define MARBLE_LOG_ENABLED 1

#if MARBLE_LOG_ENABLED

// Log type bit flags
enum LogType : uint8_t
{
    LOG_DEBUG = 0x01,
    LOG_INFO = 0x02,
    LOG_WARN = 0x04,
    LOG_ERROR = 0x08,
    LOG_WS_RECEIVE = 0x10,
    LOG_WS_SEND = 0x20
};

// Global logging configuration
class LogConfig
{
public:
    static uint8_t enabledTypes;

    static bool isEnabled(LogType type)
    {
        return (enabledTypes & type) != 0;
    }

    static void enable(LogType type)
    {
        enabledTypes |= type;
    }

    static void disable(LogType type)
    {
        enabledTypes &= ~type;
    }

    static void setAll(bool enabled)
    {
        enabledTypes = enabled ? 0xFF : 0x00;
    }
};

// Conditional logging macros

// By wrapping it in do { ... } while(0), the macro:
// - Requires a semicolon - It's a proper statement that must end with ;
// - Acts as a single statement - Works correctly in all control flow contexts (if/else, for, while)
// - No side effects - The while(0) never loops (condition is always false)
// - Gets optimized away - Compilers remove it entirely, zero runtime cost

#define MLOG_DEBUG(format, ...)                                                                         \
    do                                                                                                  \
    {                                                                                                   \
        if (LogConfig::isEnabled(LOG_DEBUG))                                                            \
        {                                                                                               \
            Serial.printf("[%6lu][D][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                               \
    } while (0)

    
#define MLOG_INFO(format, ...)                                                                          \
    do                                                                                                  \
    {                                                                                                   \
        if (LogConfig::isEnabled(LOG_INFO))                                                             \
        {                                                                                               \
            Serial.printf("[%6lu][I][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                               \
    } while (0)

#define MLOG_ERROR(format, ...)                                                                         \
    do                                                                                                  \
    {                                                                                                   \
        if (LogConfig::isEnabled(LOG_ERROR))                                                            \
        {                                                                                               \
            Serial.printf("[%6lu][E][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                               \
    } while (0)

#define MLOG_WARN(format, ...)                                                                          \
    do                                                                                                  \
    {                                                                                                   \
        if (LogConfig::isEnabled(LOG_WARN))                                                             \
        {                                                                                               \
            Serial.printf("[%6lu][W][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                               \
    } while (0)

#define MLOG_WS_SEND(format, ...)                                                                             \
    do                                                                                                        \
    {                                                                                                         \
        if (LogConfig::isEnabled(LOG_WS_SEND))                                                                \
        {                                                                                                     \
            Serial.printf("[%6lu][WS_SEND][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                                     \
    } while (0)

#define MLOG_WS_RECEIVE(format, ...)                                                                          \
    do                                                                                                        \
    {                                                                                                         \
        if (LogConfig::isEnabled(LOG_WS_RECEIVE))                                                             \
        {                                                                                                     \
            Serial.printf("[%6lu][WS_RECV][%s]: " format "\n", millis(), pcTaskGetName(NULL), ##__VA_ARGS__); \
        }                                                                                                     \
    } while (0)

#else
// Logging disabled - all macros become no-ops (zero overhead)
#define MLOG_INFO(format, ...)
#define MLOG_ERROR(format, ...)
#define MLOG_WARN(format, ...)
#define MLOG_DEBUG(format, ...)
#define MLOG_WS_SEND(format, ...)
#define MLOG_WS_RECEIVE(format, ...)
#endif

#endif // LOGGING_H
