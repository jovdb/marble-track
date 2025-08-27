#pragma once
#include <LittleFS.h>
#include "esp_log.h"

class LittleFSManager
{
public:
    LittleFSManager();
    bool setup();
    void loop();
};
