#pragma once
#include <LittleFS.h>

class LittleFSManager
{
public:
    LittleFSManager();
    bool setup();
    void loop();
};
