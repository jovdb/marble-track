import { readConfig, saveConfig, findDevice } from "../utils/configUtils.ts";
import { sendError } from "../utils/error.ts";

export function deviceSaveConfigHandler(
  ws: import("ws").WebSocket,
  deviceId: string,
  deviceConfig: object
) {
  if (!deviceId || deviceConfig === undefined) {
    sendError(ws, "Missing deviceId or config in device-save-config");
    return;
  }

  const config = readConfig();

  // Find device
  const device = findDevice(config.devices, deviceId);
  if (!device) {
    sendError(ws, `DeviceId ${deviceId} not found.`);
    return;
  }

  // Override config
  device.config = deviceConfig;

  // Save Config
  saveConfig(config);

  ws.send(
    JSON.stringify({
      type: "device-save-config",
      status: "ok",
      deviceId: deviceId,
    })
  );
}
