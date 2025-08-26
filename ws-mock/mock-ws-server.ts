import http from "http";
import { WebSocketServer } from "ws";
import { getDevicesHandler } from "./handlers/getDevicesHandler.ts";
import { deviceSaveConfigHandler } from "./handlers/deviceSaveConfigHandler.ts";
import { deviceReadConfigHandler } from "./handlers/deviceLoadConfigHandler.ts";

const PORT: number = 5173;
const WS_PATH: string = "/ws";

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
            getDevicesHandler(ws);
            break;
          case "device-save-config":
            deviceSaveConfigHandler(ws, data.deviceId, data.config);
            break;
          case "device-read-config":
            deviceReadConfigHandler(ws, data.deviceId);
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
    ws.send(JSON.stringify({ type: "hello", msg: "Mock server ready" }));
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
});
