#include "devices/WheelLoader.h"
#include "Logging.h"
#include <algorithm>

namespace devices
{
    WheelLoader::WheelLoader(const String &id)
        : Device(id, "wheelloader")
    {
        applyDefaultConfig();
    }

    WheelLoader::~WheelLoader()
    {
    }

    void WheelLoader::applyDefaultConfig()
    {
        // Internal composition: 2 servos and 2 buttons
        _innerServo = new Servo(getId() + "-inner-servo");
        addChild(_innerServo);

        _outerServo = new Servo(getId() + "-outer-servo");
        addChild(_outerServo);

        _leftButton = new Button(getId() + "-left-button");
        addChild(_leftButton);

        _rightButton = new Button(getId() + "-right-button");
        addChild(_rightButton);

        // Optional: Set default configs for children here if needed
    }

    void WheelLoader::setup()
    {
        Device::setup();
        setName(_config.name);
        MLOG_DEBUG("%s: Setup complete", toString().c_str());
    }

    void WheelLoader::loop()
    {
        Device::loop();

        if (_state.state != WheelLoaderStateEnum::IDLE && (millis() - _actionStartTime >= _actionWaitMs))
        {
            if (_state.state == WheelLoaderStateEnum::INIT)
            {
                _state.state = WheelLoaderStateEnum::IDLE;
                notifyStateChanged();
                MLOG_INFO("%s: Initialization finished", toString().c_str());
            }
            else if (_state.state == WheelLoaderStateEnum::LOADING_LEFT ||
                     _state.state == WheelLoaderStateEnum::LOADING_RIGHT ||
                     _state.state == WheelLoaderStateEnum::LOADING_ANY)
            {
                if (_sequenceStep == 0)
                {
                    // Step 1: outer to 0.5
                    _outerServo->setValue(0.5f);
                    
                    _actionWaitMs = _outerServo->getConfig().defaultDurationInMs;
                    _actionStartTime = millis();
                    _sequenceStep = 1;
                    MLOG_INFO("%s: Step 0 finished, outer returning to center", toString().c_str());
                }
                else if (_sequenceStep == 1)
                {
                    // Step 2: inner can go to target (0.0 for Left, 1.0 for Right)
                    if (_state.state == WheelLoaderStateEnum::LOADING_LEFT) {
                        _innerServo->setValue(0.0f);
                    } else {
                        _innerServo->setValue(1.0f);
                    }

                    _actionWaitMs = _innerServo->getConfig().defaultDurationInMs + 500;
                    _actionStartTime = millis();
                    _sequenceStep = 2;
                    MLOG_INFO("%s: Step 1 finished, inner moving to push position", toString().c_str());
                }
                else
                {
                    // Step 3: inner to 0.5
                    _innerServo->setValue(0.5f);
                    
                    // Sequence finished
                    _state.state = WheelLoaderStateEnum::IDLE;
                    _sequenceStep = 0;
                    notifyStateChanged();
                    MLOG_INFO("%s: Loading sequence finished", toString().c_str());
                }
            }
        }
    }

    bool WheelLoader::init()
    {
        MLOG_INFO("%s: init() called", toString().c_str());

        // Move both servos to 50%
        _innerServo->setValue(0.5f);
        _outerServo->setValue(0.5f);

        // Calculate max duration
        _actionWaitMs = std::max(_innerServo->getConfig().defaultDurationInMs,
                                 _outerServo->getConfig().defaultDurationInMs);
        _actionStartTime = millis();
        _sequenceStep = 0;

        _state.state = WheelLoaderStateEnum::INIT;
        notifyStateChanged();
        return true;
    }

    bool WheelLoader::loadLeft()
    {
        MLOG_INFO("%s: loadLeft() called", toString().c_str());
        _state.state = WheelLoaderStateEnum::LOADING_LEFT;
        _sequenceStep = 0;

        // Step 0: inner to 1 + outer to 0
        _innerServo->setValue(1.0f);
        _outerServo->setValue(0.0f);

        _actionWaitMs = _outerServo->getConfig().defaultDurationInMs + 500;
        _actionStartTime = millis();

        notifyStateChanged();
        return true;
    }

    bool WheelLoader::loadRight()
    {
        MLOG_INFO("%s: loadRight() called", toString().c_str());
        _state.state = WheelLoaderStateEnum::LOADING_RIGHT;
        _sequenceStep = 0;

        // Step 0: inner to 0 + outer to 1
        _innerServo->setValue(0.0f);
        _outerServo->setValue(1.0f);

        _actionWaitMs = _outerServo->getConfig().defaultDurationInMs + 500;
        _actionStartTime = millis();

        notifyStateChanged();
        return true;
    }

    bool WheelLoader::loadAny()
    {
        MLOG_INFO("%s: loadAny() called", toString().c_str());
        // Simple logic: if left button is pressed, load left. Else load right.
        if (_leftButton->isPressed())
        {
            return loadLeft();
        }
        else
        {
            return loadRight();
        }
    }

    bool WheelLoader::control(const String &action, JsonObject *args)
    {
        if (action == "init")
        {
            return init();
        }
        else if (action == "loadLeft")
        {
            return loadLeft();
        }
        else if (action == "loadRight")
        {
            return loadRight();
        }
        else if (action == "loadAny")
        {
            return loadAny();
        }

        MLOG_WARN("%s: Unknown action: %s", toString().c_str(), action.c_str());
        return false;
    }

    void WheelLoader::addDeviceStateToJson(JsonDocument &doc)
    {
        switch (_state.state)
        {
        case WheelLoaderStateEnum::IDLE:
            doc["state"] = "IDLE";
            break;
        case WheelLoaderStateEnum::INIT:
            doc["state"] = "INITIALIZING";
            break;
        case WheelLoaderStateEnum::LOADING_LEFT:
            doc["state"] = "LOADING_LEFT";
            break;
        case WheelLoaderStateEnum::LOADING_RIGHT:
            doc["state"] = "LOADING_RIGHT";
            break;
        case WheelLoaderStateEnum::LOADING_ANY:
            doc["state"] = "LOADING_ANY";
            break;
        case WheelLoaderStateEnum::ERROR:
            doc["state"] = "ERROR";
            break;
        default:
            doc["state"] = "UNKNOWN";
            break;
        }
    }

    void WheelLoader::jsonToConfig(const JsonDocument &config)
    {
        _config.name = config["name"] | "Wheel Loader";
    }

    void WheelLoader::configToJson(JsonDocument &doc)
    {
        doc["name"] = _config.name;
    }
} // namespace devices
