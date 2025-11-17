/**
 * @file Device.h
 * @brief Base interface for all devices
 *
 * Defines a base interface that all devices should implement to provide
 * a common base for polymorphic device management.
 *
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

using NotifyClients = std::function<void(const String &message)>;
using HasClients = std::function<bool()>;

/**
 * @class Device
 * @brief Base interface for all devices
 *
 * This class provides a common base interface for all devices in the system.
 * It allows for polymorphic device management and includes core functionality
 * like identification, naming, and loop operations.
 */
class Device
{
public:
    Device(const String &id, const String &type, NotifyClients callback = nullptr);
    virtual ~Device();
    virtual void setup();
    virtual void loop();
    void addChild(Device *child);
    std::vector<Device *> getChildren() const;
    virtual String getId() const { return _id; }
    virtual String getType() const { return _type; }
    virtual String getName() const { return _name; }
    virtual String getErrorCode() const { return _errorCode; }
    virtual String getErrorMessage() const { return _errorMessage; }
    virtual void setError(const String &code, const String &message);
    virtual void clearError();
    virtual bool control(const String &action, JsonObject *payload = nullptr);
    virtual String getState();
    virtual std::vector<int> getPins() const;
    virtual void notifyStateChange();
    virtual void notifyError(String messageType, String error);
    virtual void setConfig(JsonObject *config);
    virtual String getConfig() const;
    void setNotifyClients(NotifyClients callback);
    void setHasClients(HasClients callback);

protected:
    String _id;
    String _name;
    String _type;
    String _errorCode;
    String _errorMessage;
    std::vector<Device *> children;
    /**
     * @brief Callback function for change notifications
     */
    NotifyClients _notifyClients;
    HasClients _hasClients;
};

#endif // DEVICE_H
