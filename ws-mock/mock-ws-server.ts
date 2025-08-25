import http from "http";
import { WebSocketServer } from "ws";
import { getDevices } from "./types/get-devices.ts";
import { getDevicesHandler } from "./handlers/getDevicesHandler.ts";
import { deviceSaveConfigHandler } from "./handlers/deviceSaveConfigHandler.ts";
import fs from "fs";
import { saveConfig } from "./utils/configUtils.ts";

const PORT: number = 5173;
const WS_PATH: string = "/ws";

const server = http.createServer((req, res) => {
  res.writeHead(404);
  res.end();
});

function findDevice(devices: any, deviceId: string) {
  if (!Array.isArray(devices)) return null;
  for (const device of devices) {
    if (device.id === deviceId) return device;
    if (device.children) {
      const found = findDevice(device.children, deviceId);
      if (found) return found;
    }
  }
  return null;
}

const wss = new WebSocketServer({ noServer: true });

wss.on(
  "connection",
  (ws: import("ws").WebSocket, _request: http.IncomingMessage) => {
    console.log("Client connected");
    ws.on("message", (message: Buffer | string) => {
      const jsonStr =
        typeof message === "string" ? message : message.toString();
      console.log("Received:", jsonStr);
      try {
        const data = JSON.parse(jsonStr);
        switch (data.type) {
          case "get-devices":
            getDevicesHandler(ws);
            break;
          case "device-save-config":
            deviceSaveConfigHandler(ws, data.deviceId, data.config);
            break;
          default:
            ws.send(
              JSON.stringify({ type: "info", msg: `Unknown type ${data.type}` })
            );
        }
      } catch {
        ws.send(
          JSON.stringify({ type: "error", msg: `Invalid JSON: ${jsonStr}` })
        );
      }
    });
    ws.send(JSON.stringify({ type: "hello", msg: "Mock server ready" }));  }
);

server.on(
  "upgrade",
  (request: http.IncomingMessage, socket: any, head: Buffer) => {
    if (request.url === WS_PATH) {
      wss.handleUpgrade(request, socket, head, (ws) => {
        wss.emit("connection", ws, request);
      });
    } else {
      socket.destroy();
    }
  }
);

server.listen(PORT, () => {
  console.log(
    `Mock WebSocket server running at ws://localhost:${PORT}${WS_PATH}`
  );
});
