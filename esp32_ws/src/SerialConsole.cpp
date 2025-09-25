#include "SerialConsole.h"

#include <algorithm>
#include <limits>

#include "DeviceManager.h"
#include "Network.h"
#include "Logging.h"
#include "devices/Device.h"

SerialConsole::SerialConsole(DeviceManager &deviceManager, Network *&networkRef, OperationMode &modeRef)
    : m_deviceManager(deviceManager), m_network(networkRef), m_mode(modeRef)
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
                Serial.println("💡 Commands: 'devices', 'network', 'memory', 'restart', 'set-network'");
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
        Serial.printf("⚙️  Devices: %d total | Mode: %s\n",
                      m_deviceManager.getDeviceCount(),
                      (m_mode == OperationMode::MANUAL) ? "MANUAL" : "AUTOMATIC");

        Device *deviceList[20];
        int deviceCount = 0;
        m_deviceManager.getDevices(deviceList, deviceCount, 20);

        if (deviceCount > 0)
        {
            for (int i = 0; i < deviceCount; i++)
            {
                if (deviceList[i] != nullptr)
                {
                    Serial.printf("  %d. %s [%s] %s\n",
                                  i + 1,
                                  deviceList[i]->getType().c_str(),
                                  deviceList[i]->getId().c_str(),
                                  deviceList[i]->getName().c_str());

                    String state = deviceList[i]->getState();
                    Serial.printf("     State:  %s\n", state.c_str());

                    String config = deviceList[i]->getConfig();
                    Serial.printf("     Config: %s\n", config.c_str());

                    Serial.println();
                }
            }
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
        Serial.println("  • Type 'set-network' to configure WiFi credentials via serial.");
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

    if (input.equalsIgnoreCase("set-network") || input.equalsIgnoreCase("network-set"))
    {
        startSetNetworkFlow();
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
        cancelSetNetworkFlow();
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

void SerialConsole::logNetworkInfo()
{
    Serial.println();
    Serial.println("📡 Network Status:");

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("  ✅ WiFi: %s\n", WiFi.SSID().c_str());
        if (m_network != nullptr)
        {
            String hostname = m_network->getHostname();
            Serial.printf("  🏠 mDNS: http://%s.local\n", hostname.c_str());
        }
        else
        {
            Serial.println("  🏠 mDNS: http://marble-track.local");
        }
        Serial.printf("  🌍 Web: http://%s\n", WiFi.localIP().toString().c_str());
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

            const bool duplicate = std::any_of(m_session.networks.begin(), m_session.networks.end(), [&](const NetworkOption &existing) {
                return existing.ssid == option.ssid;
            });

            if (!duplicate)
            {
                m_session.networks.push_back(option);
            }
        }

        std::sort(m_session.networks.begin(), m_session.networks.end(), [](const NetworkOption &a, const NetworkOption &b) {
            if (a.rssi == b.rssi)
            {
                return a.ssid < b.ssid;
            }
            return a.rssi > b.rssi;
        });
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
    Serial.println("💾 Saving network settings...");

    bool saved = m_deviceManager.saveNetworkSettings(newSettings);
    if (!saved)
    {
        Serial.println("❌ Failed to write settings to /config.json.");
        m_session.reset();
        Serial.println();
        return;
    }

    Serial.println("✅ Network credentials saved to /config.json.");

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

    Serial.println();
    Serial.println("Use the 'network' command to check current status.");
    Serial.println();

    m_session.reset();
}
