ESP32:

- OTA updates
- make configurable at runtime without build
  - change pins
  - (add device)
  - save config on fs
- also support acceleration for stepper stop
- Use Async API for describing WS possibilities
- Conditional logging + configure via serial Monitor
- Conditional WS message, only when a client is connected
- Remove name property from device constructor, is config
- Allow a name /description per GPIO port
  STP: Stepper pin
  SW1: Zero detection
- Move creation of children in wheel from setup to constructor?
- Make addChild from device a protected member?

Website:

- use zod to validate state
- haertbeat - pong
- Show pins again + listen to config changes to update the list
- Upload config.json by default with deploy
- Move wheel markers to the inside

Both:

Ideas:

- Add MCP server so an AI can be connected
