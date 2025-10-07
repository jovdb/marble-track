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
  //   device.config = deviceConfig;
  device.setConfig(deviceConfig);

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
      msg: "Missing deviceId in device-state",
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

export function addDeviceHandler(
  deviceType: string,
  deviceId: string,
  deviceConfig: object
) {
  if (!deviceType || !deviceId) {
    return JSON.stringify({
      type: "add-device",
      error: "Missing deviceType or deviceId",
    });
  }

  const config = readConfig();

  // Check if device already exists
  const existingDevice = findDevice(config.devices, deviceId);
  if (existingDevice) {
    return JSON.stringify({
      type: "add-device",
      error: `DeviceId ${deviceId} already exists.`,
      deviceId: deviceId,
    });
  }

  // Create new device
  const newDevice = {
    id: deviceId,
    name: `${deviceType} ${deviceId}`,
    type: deviceType.toLowerCase(),
    config: deviceConfig || {},
  };

  // Add to devices array
  if (!config.devices) {
    config.devices = [];
  }
  config.devices.push(newDevice);

  // Save Config
  saveConfig(config);

  // Return success response
  return JSON.stringify({
    type: "add-device",
    success: true,
    deviceId: deviceId,
  });
}

export function removeDeviceHandler(deviceId: string) {
  if (!deviceId) {
    return JSON.stringify({
      type: "remove-device",
      error: "Missing deviceId",
    });
  }

  const config = readConfig();

  // Find device index
  const deviceIndex =
    config.devices?.findIndex((device: any) => device.id === deviceId) ?? -1;
  if (deviceIndex === -1) {
    return JSON.stringify({
      type: "remove-device",
      error: `DeviceId ${deviceId} not found.`,
      deviceId: deviceId,
    });
  }

  // Remove device from array
  config.devices.splice(deviceIndex, 1);

  // Save Config
  saveConfig(config);

  // Return success response
  return JSON.stringify({
    type: "remove-device",
    success: true,
    deviceId: deviceId,
  });
}

export function getDevicesConfigHandler() {
  const config = readConfig();
  if (config) {
    // Send in the format expected by the frontend: {type: "devices-config", config: {...}}
    const response = {
      type: "devices-config",
      config: config,
    };
    return JSON.stringify(response);
  } else {
    return JSON.stringify({
      type: "devices-config",
      error: "Config file not found",
    });
  }
}

export function getNetworkConfigHandler() {
  // Mock network config response
  const response = {
    type: "network-config",
    ssid: "MockNetwork",
    password: "mockpassword123",
  };
  return JSON.stringify(response);
}
