
#include "devices/DividerWheel.h"
#include "Logging.h"

DividerWheel::DividerWheel(int stepPin1, int stepPin2, int stepPin3, int stepPin4, int buttonPin, const String &id, const String &name)
    : Device(id, "wheel")
{
    _name = name;
    _stepper = new Stepper(id + "-stepper");
    // _stepper->configure4Pin(stepPin1, stepPin2, stepPin3, stepPin4);
    _button = new Button(id + "-button");
    _calibrationState = CalibrationState::NO;
    addChild(_stepper);
    addChild(_button);
}

DividerWheel::~DividerWheel()
{
    delete _stepper;
    delete _button;
}

void DividerWheel::loop()
{
    Device::loop();

    switch (_state)
    {
    case wheelState::CALIBRATING:
        if (_button->wasReleased())
        {
            MLOG_INFO("Wheel [%s]: Calibration complete.", getId().c_str());
            _stepper->setCurrentPosition(0);
            _stepper->stop();
            _state = wheelState::IDLE;
            _calibrationState = CalibrationState::YES;
            notifyStateChange();
        }
        else if (!_stepper->isMoving())
        {
            // Ended without calibrating button pressed
            // Check if button is connected
            // Check if button is pressed
            _calibrationState = CalibrationState::FAILED;
            notifyStateChange();
        }
        break;

    default:
        break;
    }
}

void DividerWheel::setup()
{
    Device::setup();
}

void DividerWheel::move(long steps)
{
    if (_stepper)
    {
        _stepper->move(steps);
    }
}

void DividerWheel::calibrate()
{
    static float calibrationSpeed = 300;
    static float calibrationAcceleration = 50;

    if (_stepper)
    {
        MLOG_INFO("Wheel [%s]: Calibration started.", getId().c_str());
        _state = wheelState::CALIBRATING;
        _stepper->setMaxSpeed(calibrationSpeed);
        _stepper->setMaxAcceleration(calibrationAcceleration);
        _stepper->move(5000);
        notifyStateChange();
    }
}

bool DividerWheel::control(const String &action, JsonObject *payload)
{
    // Use if-else if chain for string actions (C++ does not support switch on String)
    if (action == "next-breakpoint")
    {
        long steps = payload && (*payload)["steps"].is<long>() ? (*payload)["steps"].as<long>() : 5000;
        move(steps);
        return true;
    }
    else if (action == "calibrate")
    {
        calibrate();
        return true;
    }
    else if (action == "stop")
    {
        MLOG_INFO("Wheel [%s]: Stopping.", getId().c_str());
        _stepper->stop();
        return true;
    }
    // Add more actions here as needed
    return false;
}

String DividerWheel::getState()
{
    JsonDocument doc;
    // Copy base Device state fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getState());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    doc["state"] = _state == wheelState::CALIBRATING ? "CALIBRATING" : "IDLE";
    doc["calibrationState"] = _calibrationState == CalibrationState::YES  ? "YES"
                              : _calibrationState == CalibrationState::NO ? "NO"
                                                                          : "FAILED";

    String result;
    serializeJson(doc, result);
    return result;
}

String DividerWheel::getConfig() const
{
    JsonDocument doc;
    // Copy base Device config fields
    JsonDocument baseDoc;
    deserializeJson(baseDoc, Device::getConfig());
    for (JsonPair kv : baseDoc.as<JsonObject>())
    {
        doc[kv.key()] = kv.value();
    }
    // Add divider wheel-specific config if any
    String result;
    serializeJson(doc, result);
    return result;
}

void DividerWheel::setConfig(JsonObject *config)
{
    Device::setConfig(config);

    // Handle divider wheel-specific config here if needed
}
