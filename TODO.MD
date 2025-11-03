ESP32:

- OTA updates
- make configurable at runtime without build
  - change pins
  - (add device)
  - save config on fs
- Use Async API for describing WS possibilities
- Remove name property from device constructor, is config
- Allow a name /description per GPIO port
  STP: Stepper pin
  SW1: Zero detection
- Move creation of children in wheel from setup to constructor?
- Make addChild from device a protected member?
- Crate abstraction for pin / I²C expander/ I²C PWM board

Website:

- use zod to validate state
- haertbeat - pong

Both:
- ✅ Batch ws messages, allow to send an array of messages

Ideas:

- Add MCP server so an AI can be connected
