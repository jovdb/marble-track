/**
 * @file WheelLoader.h
 * @brief WheelLoader device for loading marbles onto wheels
 */

#ifndef COMPOSITION_WHEEL_LOADER_H
#define COMPOSITION_WHEEL_LOADER_H

#include "devices/Device.h"
#include "devices/Servo.h"
#include "devices/Button.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"

namespace devices
{
    /**
     * @enum WheelLoaderStateEnum
     * @brief Enumeration of wheel loader states
     */
    enum class WheelLoaderStateEnum
    {
        UNKNOWN,
        IDLE,
        INIT,
        LOADING_LEFT,
        LOADING_RIGHT,
        LOADING_ANY,
        ERROR,
    };

    /**
     * @struct WheelLoaderConfig
     * @brief Configuration for WheelLoader device
     */
    struct WheelLoaderConfig
    {
        String name = "Wheel Loader";
        float innerCenter = 0.5f;
        float outerCenter = 0.5f;
    };

    /**
     * @struct WheelLoaderState
     * @brief State structure for WheelLoader device
     */
    struct WheelLoaderState
    {
        WheelLoaderStateEnum state = WheelLoaderStateEnum::UNKNOWN;
        bool leftBallAvailable = false;
        bool rightBallAvailable = false;
    };

    /**
     * @class WheelLoader
     * @brief Device that manages loading marbles via two servos and two sensor buttons
     */
    class WheelLoader : public Device,
                        public ConfigMixin<WheelLoader, WheelLoaderConfig>,
                        public StateMixin<WheelLoader, WheelLoaderState>,
                        public ControllableMixin<WheelLoader>,
                        public SerializableMixin<WheelLoader>
    {
    public:
        explicit WheelLoader(const String &id);
        virtual ~WheelLoader();

        void setup() override;
        void loop() override;

        /**
         * @brief Initialize the loader (homing, etc.)
         */
        bool init();

        /**
         * @brief Load marble from the left side
         */
        bool loadLeft();

        /**
         * @brief Load marble from the right side
         */
        bool loadRight();

        /**
         * @brief Load marble from any available side
         */
        bool loadAny();

        // ControllableMixin implementation
        bool control(const String &action, JsonObject *args = nullptr) override;
        void addDeviceStateToJson(JsonDocument &doc) override;

        // SerializableMixin implementation
        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    protected:
        void applyDefaultConfig();

    private:
        Servo *_innerServo = nullptr;
        Servo *_outerServo = nullptr;
        Button *_leftButton = nullptr;
        Button *_rightButton = nullptr;

        unsigned long _actionStartTime = 0;
        unsigned long _actionWaitMs = 0;
        int _sequenceStep = 0;
    };
}

#endif // COMPOSITION_WHEEL_LOADER_H
