import http from "http";
import { WebSocketServer } from "ws";
import {
  getDevicesHandler,
  deviceSaveConfigHandler,
  deviceReadConfigHandler,
  getDevicesListHandler,
  deviceGetStateHandler,
  addDeviceHandler,
  removeDeviceHandler,
  getDevicesConfigHandler,
  getNetworkConfigHandler,
  getNetworksHandler,
  getNetworkStatusHandler,
  deviceControlHandler
} from "./handlers/devicesHandlers.ts";

const PORT: number = 5173;
const WS_PATH: string = "/ws";

// Helper function to log and send WebSocket messages
function sendMessage(ws: import("ws").WebSocket, message: string) {
  console.log("\x1b[34mSending:\x1b[0m", message);
  ws.send(message);
}

const server = http.createServer((req, res) => {
  res.writeHead(404);
  res.end();
});

const wss = new WebSocketServer({ noServer: true });

wss.on(
  "connection",
  (ws: import("ws").WebSocket, _request: http.IncomingMessage) => {
    console.log("Client connected");
    ws.on("message", (message: Buffer | string) => {
      const jsonStr =
        typeof message === "string" ? message : message.toString();
      console.log("\x1b[32mReceived:\x1b[0m", jsonStr);
      try {
        const data = JSON.parse(jsonStr);
        switch (data.type) {
          case "devices-list":
            sendMessage(ws, getDevicesListHandler());
            break;
          case "devices-config":
            sendMessage(ws, getDevicesConfigHandler());
            break;
          case "network-config":
            sendMessage(ws, getNetworkConfigHandler());
            break;
          case "networks":
            sendMessage(ws, getNetworksHandler());
            break;
          case "network-status":
            sendMessage(ws, getNetworkStatusHandler());
            break;
          case "device-save-config":
            sendMessage(
              ws,
              deviceSaveConfigHandler(data.deviceId, data.config)
            );
            break;
          case "device-read-config":
            sendMessage(ws, deviceReadConfigHandler(data.deviceId));
            break;
          case "device-state":
            sendMessage(ws, deviceGetStateHandler(data.deviceId));
            break;
          case "add-device":
            sendMessage(ws, addDeviceHandler(data.deviceType, data.deviceId, data.config));
            break;
          case "device-control":
            sendMessage(ws, deviceControlHandler(data.deviceId, data.action, data.payload));
            break;
          default:
            console.log("\x1b[31mUnknown message:\x1b[0m", data);
        }
      } catch {
        sendMessage(
          ws,
          JSON.stringify({ type: "error", msg: `Invalid JSON: ${jsonStr}` })
        );
      }
    });
    // sendMessage(ws, JSON.stringify({ type: "hello", msg: "Mock server ready" }));
  }
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
  console.log(
    `Set website/.env variable:\nVITE_MARBLE_WS=ws://localhost:${PORT}${WS_PATH}`
  );
});
