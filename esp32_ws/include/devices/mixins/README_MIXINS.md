# Mixin Detection System

## Overview

All mixins automatically register themselves with `Device`, allowing runtime detection of device capabilities.

## Usage

### Checking if a device has a mixin:

```cpp
Device* device = deviceManager->getCompositionDeviceById("led1");

if (device->hasMixin("controllable")) {
    // Device can be controlled via WebSocket
    device->addStateToJson(doc);
}

if (device->hasMixin("rtos")) {
    // Device runs in an RTOS task
}

if (device->hasMixin("saveable")) {
    // Device can save/load config
}

if (device->hasMixin("state")) {
    // Device tracks state with callbacks
}

if (device->hasMixin("config")) {
    // Device has typed configuration
}

if (device->hasMixin("statechange")) {
    // Device emits state change events
}
```

### Getting all mixins:

```cpp
const std::vector<String>& mixins = device->getMixins();
for (const String& mixin : mixins) {
    Serial.printf("Device has: %s\n", mixin.c_str());
}
```

## Available Mixins

| Mixin Name | Purpose | Registration |
|------------|---------|--------------|
| `controllable` | WebSocket state control | Auto-registered in constructor |
| `rtos` | FreeRTOS task support | Auto-registered in constructor |
| `saveable` | JSON config persistence | Auto-registered in constructor |
| `state` | State tracking with callbacks | Auto-registered in constructor |
| `config` | Typed configuration | Auto-registered in constructor |
| `statechange` | State change subscriptions | Auto-registered in constructor |

## Implementation Details

Each mixin registers itself automatically in its constructor using CRTP:

```cpp
template <typename Derived>
class MyMixin {
public:
    MyMixin() {
        static_cast<Derived*>(this)->registerMixin("mymixin");
    }
};
```

The base class provides:
- `void registerMixin(const String& mixinName)` - Called by mixins
- `bool hasMixin(const String& mixinName) const` - Check for mixin
- `const std::vector<String>& getMixins() const` - Get all mixins
