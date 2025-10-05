# ESP32_WS Application Architecture Overview

This diagram provides a high-level overview of the main components in the ESP32_WS firmware application.

```mermaid
graph TD
    A[Main]
    A --> B[<b>Network</b><br/>WiFi or AccessPoint<br/>OTA]
    A --> E[<b>Internal FileSystem</b><br/>LittleFS<br/>static website<br/>config.json]
    B --> I[<b>mDNS</b><br/>marble-track.info]
    B --> J[<b>OTA</b><br/>Over The Air<br/>updates]
    B --> H[<b>Website</b><br/>WebsiteHost<br/>port :80]
    E --> H
    B --> C[<b>WebSocket</b><br/>WebSocketManager<br/>port :5173]
    A --> D[<b>Device Management</b><br/>DeviceManager]
    D --> E

click J "#ota"
click I "#mdns"
click E "#littlefs"
```

## Network

## OTA

Over-the-Air (OTA) enables remote firmware updates for the ESP32 device. When OTA is initialized, the device listens for update requests over the network, allowing new firmware images to be uploaded and flashed without physical access. This streamlines maintenance and feature deployment, ensuring devices can be kept up-to-date securely and efficiently.

### mDNS

mDNS (Multicast DNS) allows the ESP32 device to advertise its hostname (`marble-track.info`) on the local network, enabling users to access the device via a human-friendly address instead of an IP. This simplifies device discovery and connection, especially in dynamic or non-static IP environments.

## LittleFS

LittleFS is a lightweight file system designed for embedded devices. On the ESP32_WS, it stores:

- static website assets
- configuration files (such as `config.json`).

This enables the firmware to serve the dashboard UI directly from onboard flash and persist device settings across reboots.
