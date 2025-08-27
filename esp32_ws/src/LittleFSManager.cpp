#include "LittleFSManager.h"
#include <LittleFS.h>
#include "esp_log.h"

static const char *TAG = "LittleFSManager";

LittleFSManager::LittleFSManager() {}

bool LittleFSManager::setup()
{
    ESP_LOGI(TAG, "Mounting file system...");
    if (!LittleFS.begin(true))
    {
        ESP_LOGE(TAG, ": ERROR mounting");
        return false;
    }
    else
    {
        ESP_LOGI(TAG, ": OK");
        return true;
    }
}

void LittleFSManager::loop()
{
    // Add any periodic LittleFS tasks here if needed
}
