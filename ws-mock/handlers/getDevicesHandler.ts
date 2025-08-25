import fs from "fs";
import { readConfig } from "../utils/configUtils.ts";

export function getDevicesHandler(ws: import("ws").WebSocket) {
  const config = readConfig();
  if (config) {
    ws.send(JSON.stringify(config));
  } else {
    ws.send(
      JSON.stringify({
        type: "error",
        msg: "Config file not found",
      })
    );
  }
}
