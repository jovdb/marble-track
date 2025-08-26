import { readConfig, findDevice } from "../utils/configUtils.ts";
import { sendError } from "../utils/error.ts";

export function deviceReadConfigHandler(
  ws: import("ws").WebSocket,
  deviceId: string
) {
  if (!deviceId) {
    sendError(ws, "Missing deviceId in device-read-config");
    return;
  }

  const config = readConfig();
  const device = findDevice(config.devices, deviceId);
  if (!device) {
    sendError(ws, `DeviceId ${deviceId} not found.`);
    return;
  }

  ws.send(
    JSON.stringify({
      type: "device-read-config",
      status: "ok",
      deviceId: deviceId,
      config: device.config ?? null,
    })
  );
}
