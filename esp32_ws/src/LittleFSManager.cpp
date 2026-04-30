#include "LittleFSManager.h"
#include <LittleFS.h>
#include "Logging.h"

LittleFSManager::LittleFSManager() {}

bool LittleFSManager::setup()
{
    MLOG_INFO("Mounting file system...");

    // Try mounting without auto-format first.
    // Passing formatOnFail=true would silently erase the entire flash partition
    // on every failed mount (e.g. after a power-loss mid-write), discarding any
    // saved configuration. Only format explicitly via formatFilesystem() when
    // the operator has confirmed that erasing is acceptable.
    if (!LittleFS.begin(false))
    {
        MLOG_ERROR(": ERROR mounting (filesystem may need formatting)");
        return false;
    }
    else
    {
        MLOG_INFO(": OK");
        return true;
    }
}

void LittleFSManager::loop()
{
    // Add any periodic LittleFS tasks here if needed
}

bool LittleFSManager::formatFilesystem()
{
    MLOG_WARN("Formatting LittleFS partition - all stored data will be erased!");
    if (!LittleFS.format())
    {
        MLOG_ERROR("LittleFS format failed");
        return false;
    }
    MLOG_INFO("LittleFS format complete");
    return true;
}
