import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "lift";
export interface ILiftState extends IDeviceState {
  state: "Unknown" | "Idle" | "BallLoaded" | "Reset" | "Error";
  currentPosition?: number;
  [key: string]: unknown;
}

export interface ILiftConfig extends IDeviceConfig {
  minSteps?: number;
  maxSteps?: number;
  [key: string]: unknown;
}

export function useLift(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<ILiftState, ILiftConfig>(deviceId);

  const up = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "up",
    });

  const down = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "down",
    });

  const reset = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "reset",
    });

  return [
    device,
    {
      ...actions,
      up,
      down,
      reset,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILiftState;
  }

  export interface IDeviceConfigs {
    [deviceType]: ILiftConfig;
  }
}
