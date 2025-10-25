import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "wheel";
export interface IWheelState extends IDeviceState {
  state: "CALIBRATING" | "IDLE";
  calibrationState: "YES" | "NO" | "FAILED";
  [key: string]: unknown;
}

export interface IWheelConfig extends IDeviceConfig {
  breakPoints: number[];
  [key: string]: unknown;
}

export function useWheel(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IWheelState, IWheelConfig>(deviceId);

  const calibrate = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "calibrate",
    });

  const reset = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "reset",
    });

  const nextBreakpoint = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "next-breakpoint",
    });

  return [
    device,
    {
      ...actions,
      calibrate,
      reset,
      nextBreakpoint,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IWheelState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IWheelConfig;
  }
}
