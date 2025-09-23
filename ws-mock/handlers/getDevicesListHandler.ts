import { readConfig } from "../utils/configUtils.ts";

export function getDevicesListHandler() {
  // Simulate a devices-list response
  const config = readConfig();
  return JSON.stringify({
    type: "devices-list",
    devices: config?.devices || [],
  });
}
