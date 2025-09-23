import { readConfig, saveConfig, findDevice } from "../utils/configUtils.ts";

export function deviceSaveConfigHandler(
  deviceId: string,
  deviceConfig: object
) {
  if (!deviceId || deviceConfig === undefined) {
    return JSON.stringify({
      type: "error",
      msg: "Missing deviceId or config in device-save-config",
    });
  }

  const config = readConfig();

  // Find device
  const device = findDevice(config.devices, deviceId);
  if (!device) {
    return JSON.stringify({
      type: "error",
      msg: `DeviceId ${deviceId} not found.`,
    });
  }

  // Override config
  device.config = deviceConfig;

  // Save Config
  saveConfig(config);

  // Return the response
  return JSON.stringify({
    type: "device-read-config",
    status: "ok",
    deviceId: deviceId,
    config: device.config ?? null,
  });
}
