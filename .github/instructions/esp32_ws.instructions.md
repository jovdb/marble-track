---
applyTo: "esp32_ws/**"
---

# Marble Track Firmware – Copilot Notes

## High-level flow

- `src/main.cpp` is the boot sequence: mount LittleFS, load `/config.json`, stand up WiFi via `Network`, attach `WebsiteHost`, create `WebSocketManager`, then call `deviceManager.setup()` with a broadcast lambda.
- Runtime loop calls `otaService.loop()`, `littleFSManager.loop()`, `network->processCaptivePortal()`, `wsManager.loop()`, and `deviceManager.loop()`; keep new work lightweight and non-blocking.

## Device patterns

- Device classes live in `src/devices/` and derive from `Device`. Implement `setup`, `loop`, `getState`, `setConfig`, and expose pins via `getPins()`.
- Register new devices inside `DeviceManager`: include the header, add creation logic in `loadDevicesFromJsonFile()`, ensure they serialize config/state correctly, and wire any custom JSON keys in `saveDevicesToJsonFile()`.
- `DeviceManager::setup()` expects you to call `broadcastState(deviceId, stateJson, "")` on updates; reuse that callback for new devices so the UI stays in sync.
- Persist config/state in LittleFS: the authoritative file is `/config.json`. `loadDevicesFromJsonFile()` and `saveDevicesToJsonFile()` already handle network credentials—preserve unknown JSON keys when writing.
- Let device action return false if it could not be executed

## WebSocket contract

- `WebSocketManager` (see `handleWebSocketMessage`) switches on the `type` field. Reuse the existing helpers (`handleDevice*`, `handleGetDevices`, etc.) or add new ones in this file.
- Responses must include a `type` string (`devices-list`, `device-config`, `devices-config`, …). Update the frontend unions in `website/src/interfaces/WebSockets.ts` whenever you introduce a new payload.
- Use `notifyClients` for broadcasts and respect the lightweight history kept in the UI (20 messages max).
- Incoming config uploads (`set-devices-config`) rewrite `/config.json` then call `deviceManager.loadDevicesFromJsonFile()`—mirror this flow for similar mutations so RAM and FS stay aligned.

## Networking & storage

- WiFi credentials live inside the same JSON file (`DeviceManager::loadNetworkSettings`). Call `saveNetworkSettings()` after validating input.
- OTA is configured through `OTA_Support.cpp`; call `otaService.setup(hostname)` only after WiFi is ready.
- Use `LittleFSManager` to mount/unmount the filesystem; never access LittleFS before `littleFSManager.setup()`.

## Build & deployment

- PlatformIO environment is `env:4d_systems_esp32s3_gen4_r8n16` (see `platformio.ini`), with LittleFS enabled and PSRAM required.
- Common commands (run from repo root):
  - `platformio run` – build firmware
  - `platformio run --target upload` – flash firmware over USB
  - `platformio run --target uploadfs` – upload LittleFS data (website assets)
- `deploy-website.ps1` automates rebuilding the SolidJS UI and pushes assets with `uploadfs`; run it before flashing when the frontend changes.
- Serial monitor defaults to 115200 baud (`test_serial.py` can help verify links on Windows).

## Logging & diagnostics

- Prefer the macros in `include/Logging.h` (`MLOG_INFO`, `MLOG_ERROR`, `MLOG_WARN`, `MLOG_WS_*`) for consistent timestamped output.
- Keep loops non-blocking; long-running work should be broken into state machines like the sample `runManualMode()` logic in `main.cpp`.
- When adding new I/O (motors, sensors), gate pin usage in config to avoid collisions and document expected JSON keys so the UI can scaffold templates.
