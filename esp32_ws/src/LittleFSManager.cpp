#include "LittleFSManager.h"
#include <LittleFS.h>
#include "Logging.h"

LittleFSManager::LittleFSManager() {}

bool LittleFSManager::setup()
{
    MLOG_INFO("Mounting file system...");
    if (!LittleFS.begin(true))
    {
        MLOG_ERROR(": ERROR mounting");
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
