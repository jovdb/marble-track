import { createMemo } from "solid-js";
import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "battery";

export interface IBatteryState extends IDeviceState {
  status?: string;
  voltage?: number;
  batteryPercent?: number;
}

export interface IBatteryConfig extends IDeviceConfig {
  name?: string;
  powerMonitorDeviceId?: string;
  minVoltage?: number; // V at 0%
  maxVoltage?: number; // V at 100%
}

export function useBattery(deviceId: string) {
  return useDevice<IBatteryState, IBatteryConfig>(deviceId);
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IBatteryState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IBatteryConfig;
  }
}
