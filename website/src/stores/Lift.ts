import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "lift";
export interface ILiftState extends IDeviceState {
  state: "Unknown" | "Idle" | "BallWaiting" | "Reset" | "Error" | "LiftLoaded" | "MovingUp" | "MovingDown";
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

  const loadBallStart = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "loadBallStart",
    });

  const loadBallEnd = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "loadBallEnd",
    });

  return [
    device,
    {
      ...actions,
      up,
      down,
      reset,
      loadBallStart,
      loadBallEnd,
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
