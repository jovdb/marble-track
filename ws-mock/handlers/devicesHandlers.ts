import { readConfig, saveConfig, findDevice } from "../utils/configUtils.ts";

export function getDevicesHandler() {
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

export function getDevicesListHandler() {
  // Simulate a devices-list response
  const config = readConfig();
  return JSON.stringify({
    type: "devices-list",
    devices: config?.devices || [],
  });
}

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

export function deviceReadConfigHandler(deviceId: string) {
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

export function deviceGetStateHandler(deviceId: string) {
  if (!deviceId) {
    return JSON.stringify({
      type: "error",
      msg: "Missing deviceId in device-get-state",
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
    type: "device-state",
    deviceId: deviceId,
    state: {},
  });
}