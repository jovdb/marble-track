ESP32:
- OTA updates
- make configurable at runtime without build
  - change pins
  - (add device)
  - save config on fs
- also support acceleration for stepper stop
- Use Async API for describing WS possibilities
- if stepper finished, wheel is not updated, 
If child state is changed, parent state is not updated

Website:
- use zod to validate state

Both:
- Add reset device in Web Ui
- Allow to configure network via accesspoint website
- Store network and password in Config not commited
- download config file via WebUi
