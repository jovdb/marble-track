# Device Composition Pattern

This folder contains the current device architecture using **composition** with mixins instead of deep inheritance hierarchies.

## Previous Approach (Removed)

The old device hierarchy that was removed:
```
Device
  └─ TaskDevice
       └─ SaveableTaskDevice
             └─ ControllableTaskDevice
```

This forced every controllable device to also be saveable and task-based, even when not needed.

## Current Composition Approach

We now use **mixins** (via CRTP - Curiously Recurring Template Pattern) to compose devices:

```cpp
class MyDevice : public DeviceBase,
                 public RtosMixin<MyDevice>,        // Optional: RTOS task
                 public SaveableMixin<MyDevice>,    // Optional: JSON config
                 public ControllableMixin<MyDevice> // Optional: WS control
{
    // Implement required methods from each mixin
};
```

## Available Mixins

### DeviceBase
Core identity and lifecycle. All devices inherit from this.

```cpp
class DeviceBase {
    String getId() const;
    String getType() const;
    String getName() const;
    void setName(const String &name);
    virtual void setup();
    virtual void loop();
    virtual std::vector<int> getPins() const;
};
```

### RtosMixin<Derived>
Adds FreeRTOS task capabilities.

**Required to implement:**
```cpp
void task();  // Main task loop (runs in separate FreeRTOS task)
```

**Provides:**
```cpp
bool startTask(taskName, stackSize, priority, core);
void stopTask();
void suspendTask();
void resumeTask();
void notifyTask();
bool isTaskRunning() const;
```

### SaveableMixin<Derived>
Adds JSON configuration persistence.

**Required to implement:**
```cpp
void loadConfigFromJson(const JsonDocument &config);
void saveConfigToJson(JsonDocument &doc) const;
```

**Provides:**
```cpp
JsonDocument getConfig() const;
void setConfig(const JsonDocument &config);
bool setupWithConfig(const JsonDocument &config);
```

### ControllableMixin<Derived>
Adds WebSocket control and state notifications.

**Required to implement:**
```cpp
bool handleControl(const String &action, JsonObject *args);
void addStateToJson(JsonDocument &doc) const;
```

**Provides:**
```cpp
bool control(const String &action, JsonObject *args);
JsonDocument getState() const;
void notifyStateChange(bool changed = true);
void notifyConfigChange(bool changed = true);
void notifyError(const String &errorType, const String &error);
void setNotifyClientsCallback(NotifyClientsCallback callback);
```

### StateChangeMixin<Derived>
Adds internal state change subscriptions (separate from WS notifications).

**Provides:**
```cpp
void onStateChange(StateChangeCallback callback);
void clearStateChangeCallbacks();

// Protected:
void emitStateChange(void *data = nullptr);
```

## Example: Full-Featured LED

```cpp
class ComposedLed : public DeviceBase,
                    public RtosMixin<ComposedLed>,
                    public SaveableMixin<ComposedLed>,
                    public ControllableMixin<ComposedLed>,
                    public StateChangeMixin<ComposedLed>
{
public:
    explicit ComposedLed(const String &id);

    // DeviceBase
    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    // RtosMixin
    void task();

    // SaveableMixin
    void loadConfigFromJson(const JsonDocument &config) override;
    void saveConfigToJson(JsonDocument &doc) const override;

    // ControllableMixin
    bool handleControl(const String &action, JsonObject *args) override;
    void addStateToJson(JsonDocument &doc) const override;

    // LED-specific
    bool set(bool state);
    bool blink(unsigned long onTime, unsigned long offTime);
};
```

## Example: Simple Sensor (No RTOS)

```cpp
class SimpleSensor : public DeviceBase,
                     public SaveableMixin<SimpleSensor>,
                     public ControllableMixin<SimpleSensor>
{
public:
    explicit SimpleSensor(const String &id);

    void setup() override;
    void loop() override;
    std::vector<int> getPins() const override;

    // SaveableMixin
    void loadConfigFromJson(const JsonDocument &config) override;
    void saveConfigToJson(JsonDocument &doc) const override;

    // ControllableMixin
    bool handleControl(const String &action, JsonObject *args) override;
    void addStateToJson(JsonDocument &doc) const override;
};
```

## Benefits

1. **Flexibility**: Compose only what you need
2. **No diamond problem**: CRTP avoids virtual inheritance issues
3. **Clear contracts**: Each mixin defines what methods to implement
4. **Testability**: Mixins can be tested in isolation
5. **Explicit dependencies**: No hidden base class requirements

## Migration Complete

The migration from inheritance to composition is now complete. All legacy TaskDevice-based classes have been removed and replaced with composition-based implementations.

## Files

- [DeviceBase.h](DeviceBase.h) - Core device class
- [RtosMixin.h](RtosMixin.h) - RTOS task support
- [SaveableMixin.h](SaveableMixin.h) - JSON config persistence
- [ControllableMixin.h](ControllableMixin.h) - WebSocket control
- [StateChangeMixin.h](StateChangeMixin.h) - Internal event subscriptions
- [ComposedLed.h](ComposedLed.h) / [ComposedLed.cpp](../../src/devices/composition/ComposedLed.cpp) - Example LED device
