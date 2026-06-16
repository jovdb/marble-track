import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "lift";
export interface ILiftState extends IDeviceState {
  state:
    | "Unknown"
    | "Error"
    | "Init"
    | "LiftDownLoading"
    | "LiftDown"
    | "LiftUpUnloading"
    | "LiftUp"
    | "MovingUp"
    | "MovingDown";
  currentPosition?: number;
  ballWaitingSince?: number;
  stepsPerSecond?: number; // Effective speed during movement (steps/s), absent when not moving
  isLoaded: boolean;
  initStep: number;
  onErrorChange: boolean;
  errorMessage?: string;
  errorCode?: "None" | "LiftConfigurationError" | "LiftStateError" | "LiftNoZero";
}

export interface ILiftConfig extends IDeviceConfig {
  minSteps?: number;
  maxSteps?: number;
}

export function useLift(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<ILiftState, ILiftConfig>(deviceId);

  const up = () => execDeviceFn("up", undefined);

  const down = () => execDeviceFn("down", undefined);

  const init = () => execDeviceFn("init", undefined);

  const loadBall = () => execDeviceFn("loadBall", undefined);

  const unloadBall = () => execDeviceFn("unloadBall", undefined);

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
