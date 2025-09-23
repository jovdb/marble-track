import fs from "fs";
import { getDevices } from "../types/get-devices.ts";

const configFileName = "config.json";

export const readConfig = (): any => {
  if (fs.existsSync(configFileName)) {
    try {
      return JSON.parse(fs.readFileSync(configFileName, "utf-8"));
    } catch {
      console.log("Error reading config file, generating new one.")
    }
  }
  // Return default config format with devices array
  const defaultMessage = getDevices();
  const config = { devices: defaultMessage.devices };
  saveConfig(config);
  return config;
};

export const saveConfig = (config: any): void => {
  fs.writeFileSync(configFileName, JSON.stringify(config, null, 2));
};

export const findDevice = (devices: any, deviceId: string): any => {
  if (!Array.isArray(devices)) return null;
  for (const device of devices) {
    if (device.id === deviceId) return device;
    if (device.children) {
      const found = findDevice(device.children, deviceId);
      if (found) return found;
    }
  }
  return null;
};
