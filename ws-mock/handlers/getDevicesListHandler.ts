import { readConfig } from "../utils/configUtils.ts";

export function getDevicesListHandler(ws: import("ws").WebSocket) {
  // Simulate a devices-list response
  const config = readConfig();
  ws.send(
    JSON.stringify({
      type: "devices-list",
      devices: config?.devices || [],
    })
  );
}
