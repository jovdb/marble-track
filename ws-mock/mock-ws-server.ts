import http from "http";
import { WebSocketServer } from "ws";
import {
  getDevicesHandler,
  deviceSaveConfigHandler,
  deviceReadConfigHandler,
  getDevicesListHandler
} from "./handlers/devicesHandlers.ts";

const PORT: number = 5173;
const WS_PATH: string = "/ws";

// Helper function to log and send WebSocket messages
function sendMessage(ws: import("ws").WebSocket, message: string) {
  console.log("Sending: ", message);
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
      console.log("Received:", jsonStr);
      try {
        const data = JSON.parse(jsonStr);
        switch (data.type) {
          case "get-devices":
//            sendMessage(ws, getDevicesHandler());
//            break;
//          case "get-devices-list":
            sendMessage(ws, getDevicesListHandler());
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
          default:
            console.log("Unknown message: ", data);
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
