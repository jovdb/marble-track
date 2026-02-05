ESP32:

- Use Async API for describing WS possibilities
- Allow a name /description per GPIO port
- Use FastAccellStepper that used PWM instead of loop
- Collect WS messages and send at end of loop as one
  - Optionally throttle the messages every Xms

Website:

- WebSocket messages types and Device.tsx
- use zod to validate state
- heartbeat - pong
- Remove  useWebsocket.ts

Both:
- ✅ Batch ws messages, allow to send an array of messages

Ideas:

- Add MCP server so an AI can be connected
