/**
 * @file Touch.h
 * @brief Touch device based on ESP32 touchRead()
 */

#ifndef COMPOSITION_TOUCH_H
#define COMPOSITION_TOUCH_H

#include "devices/Device.h"
#include "devices/mixins/StateMixin.h"
#include "devices/mixins/ConfigMixin.h"
#include "devices/mixins/ControllableMixin.h"
#include "devices/mixins/SerializableMixin.h"

namespace devices
{

    struct TouchConfig
    {
        String name = "Touch";
        int pin = -1;
        int threshold = 30000;
        unsigned long durationMs = 50;
    };

    struct TouchState
    {
        int value = 0;
        bool touched = false;
        bool isTouchedChanged = false;
    };

    class Touch : public Device,
                  public ConfigMixin<Touch, TouchConfig>,
                  public StateMixin<Touch, TouchState>,
                  public ControllableMixin<Touch>,
                  public SerializableMixin<Touch>
    {
    public:
        explicit Touch(const String &id);
        ~Touch() override;

        void setup() override;
        void teardown() override;
        void loop() override;
        std::vector<String> getPins() const override;

        void addStateToJson(JsonDocument &doc) override;
        bool control(const String &action, JsonObject *args = nullptr) override;

        void jsonToConfig(const JsonDocument &config) override;
        void configToJson(JsonDocument &doc) override;

    private:
        bool _debug = true;
        unsigned long _debugIntervalMs = 200;
        unsigned long _lastDebugAtMs = 0;
        unsigned long _plotIntervalMs = 200;
        unsigned long _lastPlotAtMs = 0;
        bool _streamValues = false;
        unsigned long _streamIntervalMs = 200;
        unsigned long _lastStreamAtMs = 0;
        bool _isSimulated = false;
        bool _simulatedTouched = false;

        bool _hasCandidate = false;
        bool _candidateTouched = false;
        unsigned long _candidateSince = 0;

        bool readTouched(int &value) const;
        bool commitTouchedState(bool touched, int value);
    };

} // namespace devices

#endif // COMPOSITION_TOUCH_H
