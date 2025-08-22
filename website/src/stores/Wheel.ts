import { createDeviceStore, IDeviceState } from "./Device";
import { IWsDeviceMessage, sendMessage } from "../hooks/useWebSocket";

const deviceType = "wheel";
export interface IWheelState extends IDeviceState {
  position: number;
  state: "CALIBRATING" | "IDLE";
  calibrationState: "YES" | "NO" | "FAILED";
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
}
