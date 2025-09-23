import fs from "fs";
import { readConfig } from "../utils/configUtils.ts";

export function getDevicesHandler(ws: import("ws").WebSocket) {
  const config = readConfig();
  if (config) {
    // Send in the format expected by the frontend: {type: "devices-list", devices: [...]}
    const response = {
      type: "devices-list",
      devices: config.devices || [],
    };
    return JSON.stringify(response);
  } else {
    return JSON.stringify({
      type: "error",
      msg: "Config file not found",
    });
  }
}
