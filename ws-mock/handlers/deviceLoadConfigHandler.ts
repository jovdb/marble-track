import { readConfig, findDevice } from "../utils/configUtils.ts";

export function deviceReadConfigHandler(
  ws: import("ws").WebSocket,
  deviceId: string
) {
  if (!deviceId) {
    return JSON.stringify({
      type: "error",
      msg: "Missing deviceId in device-read-config",
    });
  }

  const config = readConfig();
  const device = findDevice(config.devices, deviceId);
  if (!device) {
    return JSON.stringify({
      type: "error",
      msg: `DeviceId ${deviceId} not found.`,
    });
  }

  return JSON.stringify({
    type: "device-read-config",
    status: "ok",
    deviceId: deviceId,
    config: device.config ?? null,
  });
}
