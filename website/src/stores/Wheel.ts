import { createDeviceStore, IDeviceConfig, IDeviceState } from "./Device";
import { sendMessage } from "../hooks/useWebSocket";
import { IWsDeviceMessage } from "../interfaces/WebSockets";

const deviceType = "wheel";
export interface IWheelState extends IDeviceState {
  state: "CALIBRATING" | "IDLE";
  calibrationState: "YES" | "NO" | "FAILED";
}

export interface IWheelConfig extends IDeviceConfig {
  breakPoints: number[];
}

export function calibrateWheel(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "calibrate",
  } as IWsDeviceMessage);
}

export function nextBreakpoint(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "next-breakpoint",
  } as IWsDeviceMessage);
}

export function createWheelStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    calibrate: () => calibrateWheel(deviceId),
    nextBreakpoint: () => nextBreakpoint(deviceId),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IWheelState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IWheelConfig;
  }
}
