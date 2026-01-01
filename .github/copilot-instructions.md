# Marble Track – Copilot Guide

## Repository layout
- `website/` SolidJS + Vite dashboard, builds into `esp32_ws/data` so firmware can serve the assets.
- `esp32_ws/` PlatformIO Arduino firmware built around `DeviceManager`, `WebSocketManager`, and LittleFS-hosted UI.
- Root scripts (`deploy-website.ps1`, `platformio.ini`) coordinate building and flashing both halves of the system.

## Frontend essentials (`website/`)
- Stick to SolidJS conventions: don’t destructure props or store tuples; call fields as `props.field` and keep tuple accessors intact.
- Wrap UI beneath `Providers` (`Providers.tsx`) which chains `WebSocketProvider` and `DevicesProvider`; new features should consume context via `useWebSocket2()` / `useDevices()`.
- `useWebSocket2.ts` is the modern, shared socket store. Prefer its `subscribe`, `sendMessage`, and heartbeat handling; legacy `useWebSocket.ts` remains for back-compat only.
- WebSocket payload shapes live in `src/interfaces/WebSockets.ts`. Extend those unions before emitting or handling new message types.
- Device state lives in `src/stores/Devices.tsx`; extend it with `produce` updates and call `sendMessage` via the provided actions. Device-specific hooks (e.g. `stores/Led.ts`) wrap `useDevice` to expose typed commands.
- UI components mirror that data flow: list devices in `components/DevicesList.tsx`, render controls via `components/devices/*.tsx`, and reuse the shared `Device` frame.
- Styling uses CSS modules with BEM-style class names (`DevicesList.module.css`, `Device.module.css`). Create sibling `.module.css` files rather than global styles.

## Firmware essentials (`esp32_ws/`)
- Entry point `src/main.cpp` wires WiFi (`Network`), OTA, LittleFS, and `WebSocketManager` on `server(80)`. Messages broadcast back via `wsManager.notifyClients`.
- Device classes live under `src/devices/`; add new hardware by subclassing `Device`, registering it in `DeviceManager`, and updating JSON serialization in `DeviceManager.cpp`.
- Build & upload with PlatformIO (`PlatformIO Build` task or `platformio run --target uploadfs` for data, plain `run` for firmware). Board is `4d_systems_esp32s3_gen4_r8n16` with PSRAM and LittleFS.

## Workflows & tools
- Frontend dev: `npm install`, then `npm run dev` (set `VITE_MARBLE_WS` in `website/.env` to point at hardware).
- Tests: use `npx vitest run`; lint with `npm run lint` (Solid + TypeScript rules).
- Full deploy: run `deploy-website.ps1` to rebuild the UI, scrub `.env`, copy assets into LittleFS, and trigger `platformio.exe run --target uploadfs`.

## Patterns & gotchas
- WebSocket-driven flows rely on JSON command/response pairs (`add-device`, `devices-config`, etc.); always handle both success and error variants defined in the interfaces.
- Keep message history short—`useWebSocket2` trims to 20 entries; follow that approach for new logs to avoid unbounded growth.
- Device removals expect optimistic UI updates while the firmware broadcasts refreshed lists; refresh by calling the provided `loadDevices()` action rather than mutating local state.
- Builds target ESNext; avoid CommonJS patterns and ensure any new dependencies support ESM.
