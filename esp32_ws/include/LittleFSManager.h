#pragma once
#include <LittleFS.h>

class LittleFSManager
{
public:
    LittleFSManager();
    bool setup();
    void loop();

    /**
     * @brief Explicitly erase and reformat the LittleFS partition.
     *
     * Call this only when the operator has confirmed that all stored data
     * (e.g. device configuration) can be discarded. After formatting, call
     * setup() again to remount the freshly-formatted filesystem.
     *
     * @return true if formatting succeeded, false otherwise.
     */
    bool formatFilesystem();
};
