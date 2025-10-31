#include "AutoMode.h"
#include "Logging.h"

AutoMode::AutoMode(DeviceManager &deviceManager) : deviceManager(deviceManager)
{
}

void AutoMode::setup()
{
    MLOG_INFO("AutoMode setup complete");
    // Add any initialization logic here that should run after all devices are set up
}

void AutoMode::loop()
{
    // Add automatic mode logic here
    // This will be called in the main loop when in AUTOMATIC mode
}