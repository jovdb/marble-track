#ifndef SERIALCONSOLE_H
#define SERIALCONSOLE_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "OperationMode.h"
#include "NetworkSettings.h"

class DeviceManager;
class Network;

class SerialConsole
{
public:
    SerialConsole(DeviceManager &deviceManager, Network *&networkRef, OperationMode &modeRef);

    void loop();

private:
    DeviceManager &m_deviceManager;
    Network *&m_network;
    OperationMode &m_mode;

    struct NetworkOption
    {
        String ssid;
        int32_t rssi;
        wifi_auth_mode_t authMode;
    };

    struct Session
    {
        enum class State
        {
            Idle,
            SelectingNetwork,
            EnteringCustomSsid,
            EnteringPassword,
            Confirming,
            DeletingDevice
        };

        State state = State::Idle;
        std::vector<NetworkOption> networks;
        std::vector<String> deviceIds;
        String stageBuffer;
        String selectedSsid;
        String password;

        void reset()
        {
            state = State::Idle;
            networks.clear();
            deviceIds.clear();
            stageBuffer = "";
            selectedSsid = "";
            password = "";
        }
    };

    Session m_session;
    String m_commandBuffer;

    void logNetworkInfo();
    void handleInteractiveInput(char incoming);
    void handleCommand(const String &input);
    void handleSelectingNetworkInput(char incoming);
    void handleCustomSsidInput(char incoming);
    void handlePasswordInput(char incoming);
    void handleConfirmationInput(char incoming);
    void handleDeviceDeletionInput(char incoming);

    void startSetNetworkFlow();
    void cancelSetNetworkFlow(const char *reason = nullptr);
    void showConfirmationPrompt();
    void finishNetworkSelection(size_t selectedIndex);
    void saveAndApplyNetworkSettings();
    void startDeleteDeviceFlow();
    void cancelDeviceDeletion();

    static bool isBackspace(char incoming);
    static bool isLineFeed(char incoming);
    static bool isEscape(char incoming);
    bool handleBackspace(String &buffer);
};

#endif // SERIALCONSOLE_H
