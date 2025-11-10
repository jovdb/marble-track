import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "lift";
export interface ILiftState extends IDeviceState {
  state:
    | "Unknown"
    | "Error"
    | "Reset"
    | "LiftDownLoading"
    | "LiftDownLoaded"
    | "LiftUpUnloading"
    | "LiftUpUnloaded"
    | "LiftUpLoaded"
    | "LiftDownUnloaded"
    | "MovingUp"
    | "MovingDown";
  currentPosition?: number;
  isBallWaiting: boolean;
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

  const init = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "init",
    });

  const loadBall = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "loadBall",
    });

  const unloadBall = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "unloadBall",
    });

  return [
    device,
    {
      ...actions,
      up,
      down,
      init,
      loadBall,
      unloadBall,
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
