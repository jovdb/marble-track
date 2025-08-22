import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "./Device";
import { IWsDeviceMessage, sendMessage } from "../hooks/useWebSocket";

export interface IWheelState extends IDeviceState {
  position: number;
  state: "CALIBRATING" | "IDLE";
  calibrationState: "YES" | "NO" | "FAILED";
}

export function calibrateWheel(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType: "wheel",
    deviceId,
    fn: "calibrate",
  } as IWsDeviceMessage);
}

export function nextBreakpoint(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType: "wheel",
    deviceId,
    fn: "next-breakpoint",
  } as IWsDeviceMessage);
}

export function createWheelStore(deviceId: string) {
  const wheel = createDeviceStore<IWheelState, unknown>(`${deviceId}`);

  return {
    ...wheel,
    calibrate: () => calibrateWheel(deviceId),
    nextBreakpoint: () => nextBreakpoint(deviceId),
  };
}
