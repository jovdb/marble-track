/**
 * @file Logging.cpp
 * @brief Implementation of logging configuration
 */

#include "Logging.h"

#if MARBLE_LOG_ENABLED
// Initialize with all log types enabled by default
uint8_t LogConfig::enabledTypes = LOG_DEBUG | LOG_INFO | LOG_WARN | LOG_ERROR; // | LOG_WS_RECEIVE | LOG_WS_SEND;
#endif
