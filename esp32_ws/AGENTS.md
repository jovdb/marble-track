# Marble Track ESP32 Firmware – Agent Guide

## Repository layout

- `src/main.cpp` Entry point: initializes WiFi, WebSocket, devices, and OTA.
- `src/devices/` Device classes (Wheel, Lift, Led, etc.) inheriting from `Device`.
- `include/` Headers for devices, managers, and utilities.
- `data/` Excluded from repo, this folder will up uploaded to the esp32 LittleFS, contains mainly the website
- `config.json` (in LittleFS) Stores device configs and network settings.
- PlatformIO config in root `platformio.ini` for build/flash.

## Firmware essentials

- Device base class: Implement `setup()`, `loop()`, `getPins()`, and JSON serialization.
- DeviceManager: Loads/saves devices from/to `config.json`, broadcasts state via WebSocket.
- WebSocketManager: Handles messages like `get-devices`, `set-device-config`; responses include `type` field.
- MCPWM management: `McPwmChannels` for channels (Servo), static counter for units (PwmStepper).
- Logging: Use `MLOG_*` macros for consistent output.

## Device patterns

- Each device has config (pins, params) and state (position, error).
- Setup: Validate pins, initialize hardware (e.g., FastAccelStepper for motors).
- Loop: Handle state machines (e.g., Wheel: IDLE/MOVING/INIT).
- Control: Actions like "move", "stop"; return false on failure.
- Serialization: `jsonToConfig()`, `configToJson()`, `addStateToJson()` for WebSocket/FS.

## WebSocket contract

- Messages: `{"type": "get-devices"}` → `{"type": "devices-list", "devices": [...]}`.
- Device actions: `{"type": "device-control", "deviceId": "...", "action": "..."}`.
- Config updates: Rewrite `config.json`, reload devices.
- History: Keep lightweight, max 20 messages in UI.

## Networking & storage

- WiFi: Credentials in `config.json`; captive portal for setup.
- LittleFS: Mount via `LittleFSManager`; store configs/assets.
- OTA: Enabled after WiFi; hostname from config.

## Build & deployment

- PlatformIO: `pio run` build, `pio run --target upload` flash firmware, `uploadfs` for LittleFS.
- `deploy-website.ps1`: Builds UI, uploads assets.
- Serial: 115200 baud for logs.

## Pins

- Non-blocking loops: Use state machines for long ops.
- Pins: There is a pin abstraction: Pins.cpp
  It has 2 types on pins:
  - Onboards GPIO pins
  - Pins from I2C Expander boards
- To get unique PWM channels we have 2 files:
  - McPwmChannels: for unique MCPWM channels
  - LedcChannels: for unique LedC channels

## Patterns & gotchas

- Non-blocking loops: Use state machines for long ops.
- Error handling: Set error state, log with `MLOG_ERROR`.
- JSON: Preserve unknown keys in config; use ArduinoJson.
- Device creation: Add to `DeviceManager::loadDevicesFromJsonFile()`.

# Device state

For Device that have an error state, also return:

- errorCode: can be used for identification (play error specific sound)
- Can be shown in the website errorMessage
