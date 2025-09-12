# GitHub Copilot Instructions for esp32_ws

## Project Overview

This project is the firmware for the ESP32-based marble track system. It uses PlatformIO and the Arduino framework.

## Coding Guidelines

- Use pointer-based device management for all composite devices.
- All device constructors should accept direct pin numbers (no config files).
- Use FastAccelStepper for stepper motors (supports both 2-pin and 4-pin configurations).
- Composite devices (e.g., Wheel, GateWithSensor) should add their child devices using `addChild()`.
- Override `setup()` and `loop()` as needed, but propagate calls to children.
- Use `setStateChangeCallback()` for device state updates and event handling.
- Register all devices in `main.cpp` using `deviceManager.addDevice()`.

## File Structure

- `include/devices/`: Device headers (Button, Servo, Buzzer, Stepper, Wheel, GateWithSensor)
- `src/devices/`: Device implementations
- `main.cpp`: Device instantiation, event loop, registration
- `platformio.ini`: PlatformIO configuration

## Build & Flash

- Use PlatformIO tasks to build, upload, and monitor the firmware.
  Instead of using `poi` command , use the full poi path `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe`
- Example commands:
  - Build: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe run`
  - Upload: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe run --target upload`
  - Monitor: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe device monitor`
- When adjusting code, always start a build to see if there are build errors, if so try to fix them and rebuild until all fixed.

## Adding New Devices

1. Create header/source files in `include/devices/` and `src/devices/`.
2. Inherit from `Device` and implement required methods.
3. For composite devices, manage children with pointers and `addChild()`.
4. Register the device in `main.cpp`.

## Scripts

- Monitor logs: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe device monitor`
- Build: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe run`
- Upload code: `C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe run --target upload`

## Troubleshooting

- If build errors occur, Check the logging output to know what iswrong
- For runtime errors, check the logging of the PlatformIO Serial Monitor

For further assistance, use GitHub Copilot in VS Code and refer to this file for project-specific instructions.
