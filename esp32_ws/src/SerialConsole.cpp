#include "SerialConsole.h"

#include <algorithm>
#include <limits>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "DeviceManager.h"
#include "Network.h"
#include "Logging.h"
#include "devices/Device.h"

SerialConsole::SerialConsole(DeviceManager &deviceManager, Network *&networkRef, AutoMode *&autoModeRef, ManualMode *&manualModeRef)
    : m_deviceManager(deviceManager), m_network(networkRef), m_autoMode(autoModeRef), m_manualMode(manualModeRef)
{
}

bool SerialConsole::isBackspace(char incoming)
{
    return incoming == 8 || incoming == 127;
}

bool SerialConsole::isLineFeed(char incoming)
{
    return incoming == '\n';
}

bool SerialConsole::isEscape(char incoming)
{
    return incoming == 27;
}

bool SerialConsole::handleBackspace(String &buffer)
{
    if (buffer.isEmpty())
    {
        return false;
    }

    buffer.remove(buffer.length() - 1);
    Serial.print("\b \b");
    return true;
}

void SerialConsole::loop()
{
    while (Serial.available() > 0)
    {
        char incoming = static_cast<char>(Serial.read());

        if (m_session.state != Session::State::Idle)
        {
            handleInteractiveInput(incoming);
            continue;
        }

        if (incoming == '\r')
        {
            continue;
        }

        if (incoming == 27)
        {
            m_commandBuffer = "";
            Serial.println();
            Serial.println("❎ Command entry cancelled.");
            Serial.println();
            continue;
        }

        if (incoming == '\n')
        {
            String input = m_commandBuffer;
            m_commandBuffer = "";
            input.trim();

            Serial.println();

            if (input.length() == 0)
            {
                Serial.println("💡 Commands: 'devices', 'network', 'memory', 'config', 'version', 'logging', 'restart'");
                Serial.println();
                continue;
            }

            handleCommand(input);
            continue;
        }

        if (isBackspace(incoming))
        {
            if (!m_commandBuffer.isEmpty())
            {
                m_commandBuffer.remove(m_commandBuffer.length() - 1);
                Serial.print("\b \b");
            }
            continue;
        }

        m_commandBuffer += incoming;
        Serial.print(incoming);
    }
}

void SerialConsole::handleCommand(const String &input)
{
    if (input.equalsIgnoreCase("restart"))
    {
        Serial.println("🔄 Restarting ESP32...");
        delay(1000);
        ESP.restart();
        return;
    }

    if (input.equalsIgnoreCase("devices"))
    {
        std::vector<Device*> allDevices = m_deviceManager.getAllDevices();

        Serial.printf("⚙️  Devices: %d total | Mode: %s\n",
                      static_cast<int>(allDevices.size()),
                      m_manualMode ? "MANUAL" : "AUTOMATIC");

        if (!allDevices.empty())
        {
            for (size_t i = 0; i < allDevices.size(); i++)
            {
                if (allDevices[i] != nullptr)
                {
                    Serial.printf("  %d. %s [%s] %s\n",
                                  static_cast<int>(i + 1),
                                  allDevices[i]->getType().c_str(),
                                  allDevices[i]->getId().c_str(),
                                  allDevices[i]->getName().c_str());

                    // State and config not available on Device
                    // TODO: Re-enable when devices implement IControllable/ISerializable
                    // String state = allDevices[i]->getState();
                    // Serial.printf("     State:  %s\n", state.c_str());
                    // String config = allDevices[i]->getConfig();
                    // Serial.printf("     Config: %s\n", config.c_str());

                    // Show children IDs
                    auto children = allDevices[i]->getChildren();
                    if (!children.empty())
                    {
                        Serial.printf("     Children: ");
                        for (size_t j = 0; j < children.size(); ++j)
                        {
                            if (children[j])
                            {
                                Serial.printf("%s", children[j]->getId().c_str());
                                if (j < children.size() - 1)
                                {
                                    Serial.printf(", ");
                                }
                            }
                        }
                        Serial.println();
                    }

                    Serial.println();
                }
            }

            Serial.printf("To remove a device, type 'devices-del'.\n");
        }
        else
        {
            Serial.println("  No devices found");
        }
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("network"))
    {
        logNetworkInfo();
        Serial.println("  • Type 'network-set' to configure WiFi credentials via serial.");
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("memory"))
    {
        Serial.println("💾 Memory Status:");
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t percent = (freeHeap * 100) / totalHeap;
        Serial.printf("   ⚡ Free: %d%% (%d bytes) | Total: %d bytes\n", percent, freeHeap, totalHeap);
        Serial.printf("   📈 Min Free: %d bytes | CPU: %d MHz\n", ESP.getMinFreeHeap(), ESP.getCpuFreqMHz());
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("version"))
    {
        Serial.println("🏗️  Build Information:");
        Serial.printf("   📅 Build Date: %s\n", __DATE__);
        Serial.printf("   🕐 Build Time: %s\n", __TIME__);
        Serial.printf("   🔧 ESP32 Chip: %s\n", ESP.getChipModel());
        Serial.printf("   📊 Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("config"))
    {
        Serial.println("📄 Configuration File:");

        // Read config.json from LittleFS
        File file = LittleFS.open("/config.json", "r");
        if (!file)
        {
            Serial.println("❌ config.json not found");
        }
        else
        {
            // Parse file contents as JSON
            JsonDocument configDoc;
            DeserializationError err = deserializeJson(configDoc, file);
            if (err)
            {
                Serial.println("❌ Failed to parse config.json");
            }
            else
            {
                // Pretty print the JSON
                String configStr;
                serializeJsonPretty(configDoc, configStr);
                Serial.println(configStr);
            }
            file.close();
        }
        Serial.println();
        return;
    }

    if (input.equalsIgnoreCase("network-set") || input.equalsIgnoreCase("set-network"))
    {
        startSetNetworkFlow();
        return;
    }

    if (input.equalsIgnoreCase("devices-del"))
    {
        startDeleteDeviceFlow();
        return;
    }

    if (input.equalsIgnoreCase("logging"))
    {
        startLoggingMenu();
        return;
    }

    Serial.printf("❓ Unknown command: '%s'\n", input.c_str());
    Serial.println();
}

void SerialConsole::handleInteractiveInput(char incoming)
{
    using State = Session::State;

    if (isEscape(incoming))
    {
        if (m_session.state == State::DeletingDevice)
        {
            cancelDeviceDeletion();
        }
        else
        {
            cancelSetNetworkFlow();
        }
        return;
    }

    if (incoming == '\r')
    {
        return;
    }

    switch (m_session.state)
    {
    case State::SelectingNetwork:
        handleSelectingNetworkInput(incoming);
        break;
    case State::EnteringCustomSsid:
        handleCustomSsidInput(incoming);
        break;
    case State::EnteringPassword:
        handlePasswordInput(incoming);
        break;
    case State::Confirming:
        handleConfirmationInput(incoming);
        break;
    case State::DeletingDevice:
        handleDeviceDeletionInput(incoming);
        break;
    case State::LoggingMenu:
        handleLoggingMenuInput(incoming);
        break;
    case State::Idle:
    default:
        break;
    }
}

void SerialConsole::handleSelectingNetworkInput(char incoming)
{
    if (isLineFeed(incoming))
    {
        Serial.println();
        if (m_session.stageBuffer.isEmpty())
        {
            Serial.print("Select network #: ");
            return;
        }

        int choice = m_session.stageBuffer.toInt();
        m_session.stageBuffer = "";

        if (choice == 0)
        {
            Serial.println("Manual SSID entry selected.");
            Serial.println("Enter the SSID and press Enter (Esc to cancel).");
            Serial.print("SSID: ");
            m_session.state = Session::State::EnteringCustomSsid;
            return;
        }

        size_t index = (choice > 0) ? static_cast<size_t>(choice - 1) : std::numeric_limits<size_t>::max();
        if (index < m_session.networks.size())
        {
            finishNetworkSelection(index);
            return;
        }

        Serial.println("❌ Invalid selection. Try again.");
        Serial.print("Select network #: ");
        return;
    }

    if (isBackspace(incoming))
    {
        handleBackspace(m_session.stageBuffer);
        return;
    }

    if (isDigit(incoming))
    {
        m_session.stageBuffer += incoming;
        Serial.print(incoming);
    }
}

void SerialConsole::handleCustomSsidInput(char incoming)
{
    if (isLineFeed(incoming))
    {
        if (m_session.stageBuffer.isEmpty())
        {
            Serial.println();
            Serial.println("SSID cannot be empty. Try again.");
            Serial.print("SSID: ");
            return;
        }

        m_session.selectedSsid = m_session.stageBuffer;
        m_session.stageBuffer = "";
        Serial.println();
        Serial.println("Enter the WiFi password (leave empty for open network, Esc to cancel).");
        Serial.print("Password: ");
        m_session.password = "";
        m_session.state = Session::State::EnteringPassword;
        return;
    }

    if (isBackspace(incoming))
    {
        handleBackspace(m_session.stageBuffer);
        return;
    }

    m_session.stageBuffer += incoming;
    Serial.print(incoming);
}

void SerialConsole::handlePasswordInput(char incoming)
{
    if (isLineFeed(incoming))
    {
        m_session.password = m_session.stageBuffer;
        m_session.stageBuffer = "";
        Serial.println();
        showConfirmationPrompt();
        return;
    }

    if (isBackspace(incoming))
    {
        handleBackspace(m_session.stageBuffer);
        return;
    }

    m_session.stageBuffer += incoming;
    Serial.print('*');
}

void SerialConsole::handleConfirmationInput(char incoming)
{
    if (isLineFeed(incoming))
    {
        saveAndApplyNetworkSettings();
    }
}

void SerialConsole::handleDeviceDeletionInput(char incoming)
{
    if (isLineFeed(incoming))
    {
        Serial.println();

        if (m_session.stageBuffer.isEmpty())
        {
            Serial.print("Select device #: ");
            return;
        }

        int choice = m_session.stageBuffer.toInt();
        m_session.stageBuffer = "";

        if (choice < 1 || choice > static_cast<int>(m_session.deviceIds.size()))
        {
            Serial.println("❌ Invalid selection. Try again.");
            Serial.print("Select device #: ");
            return;
        }

        const String &deviceId = m_session.deviceIds[static_cast<size_t>(choice - 1)];

        if (!m_deviceManager.removeDevice(deviceId))
        {
            Serial.printf("❌ Failed to remove device '%s'.\n", deviceId.c_str());
            Serial.println();
            m_session.reset();
            return;
        }

        m_deviceManager.saveDevicesToJsonFile();

        Serial.printf("✅ Device '%s' removed and saved.\n", deviceId.c_str());
        Serial.println();

        m_session.reset();
        return;
    }

    if (isBackspace(incoming))
    {
        handleBackspace(m_session.stageBuffer);
        return;
    }

    if (isDigit(incoming))
    {
        m_session.stageBuffer += incoming;
        Serial.print(incoming);
    }
}

void SerialConsole::logNetworkInfo()
{
    Serial.println();
    Serial.println("📡 Network Status:");

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("  ✅ WiFi: %s\n", WiFi.SSID().c_str());
        Serial.printf("  🏠 IP  : http://%s\n", WiFi.localIP().toString().c_str());
        String hostname = m_network->getHostname();
        if (m_network != nullptr)
        {
            Serial.printf("  🌍 URL : http://%s.local\n", hostname.c_str());
            Serial.printf("  🔗 WS  : ws://%s.local/ws\n", hostname.c_str());
        }
        else
        {
            Serial.println("  🌍 URL : http://marble-track.local");
            Serial.printf("  🔗 WS  : ws://marble-track.local/ws\n", hostname.c_str());
        }
    }
    else
    {
        Serial.println("  ❌ WiFi: Disconnected");
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
        {
            Serial.printf("  🏢 AP Mode: %s at %s\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
        }
    }
    Serial.println();
}

void SerialConsole::startSetNetworkFlow()
{
    m_session.reset();

    Serial.println();
    Serial.println("🔍 Scanning for WiFi networks...");

    int16_t networkCount = WiFi.scanNetworks(false, false);

    constexpr size_t MAX_OPTIONS = 10;
    if (networkCount > 0)
    {
        const size_t reserveCount = std::min(static_cast<size_t>(networkCount), MAX_OPTIONS);
        m_session.networks.reserve(reserveCount);
        for (int16_t idx = 0; idx < networkCount && m_session.networks.size() < MAX_OPTIONS; ++idx)
        {
            NetworkOption option{WiFi.SSID(idx), WiFi.RSSI(idx), WiFi.encryptionType(idx)};

            if (option.ssid.isEmpty())
            {
                continue;
            }

            const bool duplicate = std::any_of(m_session.networks.begin(), m_session.networks.end(), [&](const NetworkOption &existing)
                                               { return existing.ssid == option.ssid; });

            if (!duplicate)
            {
                m_session.networks.push_back(option);
            }
        }

        std::sort(m_session.networks.begin(), m_session.networks.end(), [](const NetworkOption &a, const NetworkOption &b)
                  {
            if (a.rssi == b.rssi)
            {
                return a.ssid < b.ssid;
            }
            return a.rssi > b.rssi; });
    }

    WiFi.scanDelete();

    if (m_session.networks.empty())
    {
        Serial.println("⚠️  No WiFi networks detected.");
        Serial.println("Enter the SSID manually and press Enter (Esc to cancel).");
        Serial.println();
        Serial.print("SSID: ");
        m_session.state = Session::State::EnteringCustomSsid;
        m_session.stageBuffer = "";
        return;
    }

    Serial.println();
    Serial.println("Available networks:");
    for (size_t optionIdx = 0; optionIdx < m_session.networks.size(); ++optionIdx)
    {
        const auto &opt = m_session.networks[optionIdx];
        Serial.printf("  %d. %s (%d dBm)%s\n",
                      static_cast<int>(optionIdx + 1),
                      opt.ssid.c_str(),
                      opt.rssi,
                      opt.authMode == WIFI_AUTH_OPEN ? " [open]" : "");
    }
    Serial.println("  0. Enter SSID manually");
    Serial.println();
    Serial.println("Type the number of the network and press Enter.");
    Serial.println("Press Esc at any time to cancel.");

    m_session.state = Session::State::SelectingNetwork;
    m_session.stageBuffer = "";
}

void SerialConsole::cancelSetNetworkFlow(const char *reason)
{
    Serial.println();
    if (reason && reason[0] != '\0')
    {
        Serial.printf("⚠️  %s\n", reason);
    }
    Serial.println("❎ Network configuration cancelled.");
    Serial.println();
    m_session.reset();
}

void SerialConsole::showConfirmationPrompt()
{
    Serial.println();
    Serial.println("📦 Network configuration preview:");
    Serial.printf("  SSID: %s\n", m_session.selectedSsid.c_str());
    if (m_session.password.length() > 0)
    {
        String masked(m_session.password.length(), '*');
        Serial.printf("  Password: %s\n", masked.c_str());
    }
    else
    {
        Serial.println("  Password: (open network)");
    }
    Serial.println();
    Serial.println("Press Enter to save these settings, or Esc to cancel.");
    m_session.state = Session::State::Confirming;
}

void SerialConsole::finishNetworkSelection(size_t selectedIndex)
{
    m_session.selectedSsid = m_session.networks[selectedIndex].ssid;

    Serial.printf("Selected network: %s\n", m_session.selectedSsid.c_str());

    if (m_session.networks[selectedIndex].authMode == WIFI_AUTH_OPEN)
    {
        Serial.println("This network is open. No password required.");
        m_session.password = "";
        showConfirmationPrompt();
        return;
    }

    Serial.println("Enter the WiFi password (Esc to cancel).");
    Serial.print("Password: ");
    m_session.stageBuffer = "";
    m_session.password = "";
    m_session.state = Session::State::EnteringPassword;
}

void SerialConsole::saveAndApplyNetworkSettings()
{
    if (m_session.selectedSsid.isEmpty())
    {
        cancelSetNetworkFlow("SSID cannot be empty.");
        return;
    }

    NetworkSettings newSettings(m_session.selectedSsid, m_session.password);

    Serial.println();
    Serial.println("� Applying network settings...");

    if (m_network != nullptr)
    {
        NetworkMode result = m_network->applySettings(newSettings);
        switch (result)
        {
        case NetworkMode::WIFI_CLIENT:
            Serial.printf("✅ Connected to '%s' at %s\n",
                          WiFi.SSID().c_str(),
                          WiFi.localIP().toString().c_str());
            break;
        case NetworkMode::ACCESS_POINT:
            Serial.println("⚠️  Could not join the WiFi network. Fallback Access Point is active.");
            Serial.printf("     Connect to %s at %s to retry.\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
            break;
        case NetworkMode::DISCONNECTED:
        default:
            Serial.println("❌ Network connection failed. Check the credentials and try again.");
            break;
        }
    }
    else
    {
        Serial.println("⚠️  Network manager is not initialized yet. Settings will apply on next reboot.");
    }

    Serial.println("💾 Saving network settings to config file...");

    bool saved = m_deviceManager.saveNetworkSettings(newSettings);
    if (!saved)
    {
        Serial.println("❌ Failed to write settings to /config.json.");
        Serial.println("⚠️  Network settings applied but not saved. They will be lost on reboot.");
    }
    else
    {
        Serial.println("✅ Network credentials saved to /config.json.");
    }

    Serial.println();
    Serial.println("Use the 'network' command to check current status.");
    Serial.println();

    m_session.reset();
}

void SerialConsole::startDeleteDeviceFlow()
{
    Device *deviceList[20];
    int deviceCount = 0;
    m_deviceManager.getDevices(deviceList, deviceCount, 20);

    Serial.println();

    if (deviceCount == 0)
    {
        Serial.println("⚠️  No devices configured. Nothing to delete.");
        Serial.println();
        return;
    }

    m_session.reset();
    m_session.stageBuffer = "";

    Serial.println("🗑️  Delete a device:");
    for (int i = 0; i < deviceCount; ++i)
    {
        if (deviceList[i] != nullptr)
        {
            m_session.deviceIds.push_back(deviceList[i]->getId());
            Serial.printf("  %d. %s [%s]\n",
                          i + 1,
                          deviceList[i]->getType().c_str(),
                          deviceList[i]->getId().c_str());
        }
    }

    Serial.println();
    Serial.println("Type the number of the device to remove and press Enter.");
    Serial.println("Press Esc to cancel.");
    Serial.print("Select device #: ");

    m_session.state = Session::State::DeletingDevice;
}

void SerialConsole::cancelDeviceDeletion()
{
    Serial.println();
    Serial.println("❎ Device deletion cancelled.");
    Serial.println();
    m_session.reset();
}

void SerialConsole::startLoggingMenu()
{
    m_session.reset();
    m_session.state = Session::State::LoggingMenu;
    showLoggingMenu();
}

void SerialConsole::showLoggingMenu()
{
    Serial.println();
    Serial.println("📋 Logging Configuration:");
    Serial.printf("  1. DEBUG      : %s\n", LogConfig::isEnabled(LOG_DEBUG) ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("  2. INFO       : %s\n", LogConfig::isEnabled(LOG_INFO) ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("  3. WARN       : %s\n", LogConfig::isEnabled(LOG_WARN) ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("  4. ERROR      : %s\n", LogConfig::isEnabled(LOG_ERROR) ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("  5. WS_RECEIVE : %s\n", LogConfig::isEnabled(LOG_WS_RECEIVE) ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("  6. WS_SEND    : %s\n", LogConfig::isEnabled(LOG_WS_SEND) ? "✅ Enabled" : "❌ Disabled");
    Serial.println();
    Serial.println("Press 1-6 to toggle, 'a' for all, 'n' for none, Enter or Esc to exit.");
    Serial.println();
}

void SerialConsole::handleLoggingMenuInput(char incoming)
{
    if (isEscape(incoming))
    {
        cancelLoggingMenu();
        return;
    }

    if (incoming == '\r')
    {
        return;
    }

    if (incoming == '\n')
    {
        cancelLoggingMenu();
        return;
    }

    LogType type;
    String typeName;
    bool validInput = true;

    switch (incoming)
    {
        case '1':
            type = LOG_DEBUG;
            typeName = "DEBUG";
            break;
        case '2':
            type = LOG_INFO;
            typeName = "INFO";
            break;
        case '3':
            type = LOG_WARN;
            typeName = "WARN";
            break;
        case '4':
            type = LOG_ERROR;
            typeName = "ERROR";
            break;
        case '5':
            type = LOG_WS_RECEIVE;
            typeName = "WS_RECEIVE";
            break;
        case '6':
            type = LOG_WS_SEND;
            typeName = "WS_SEND";
            break;
        case 'a':
        case 'A':
            LogConfig::setAll(true);
            Serial.println("✅ All logging types enabled");
            showLoggingMenu();
            return;
        case 'n':
        case 'N':
            LogConfig::setAll(false);
            Serial.println("❌ All logging types disabled");
            showLoggingMenu();
            return;
        default:
            validInput = false;
            break;
    }

    if (validInput)
    {
        if (LogConfig::isEnabled(type))
        {
            LogConfig::disable(type);
            Serial.printf("❌ %s logging disabled\n", typeName.c_str());
        }
        else
        {
            LogConfig::enable(type);
            Serial.printf("✅ %s logging enabled\n", typeName.c_str());
        }
        showLoggingMenu();
    }
}

void SerialConsole::cancelLoggingMenu()
{
    Serial.println();
    Serial.println("❎ Logging menu closed.");
    Serial.println();
    m_session.reset();
}

