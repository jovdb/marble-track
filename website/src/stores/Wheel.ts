import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "wheel";
export interface IWheelState extends IDeviceState {
  state: "UNKNOWN" | "CALIBRATING" | "IDLE" | "INIT" | "MOVING" | "ERROR";
  lastZeroPosition: number;
  stepsInLastRevolution: number;
  currentAngle: number;
  targetAngle?: number;
  speedRpm?: number;
  acceleration?: number;
  currentBreakpointIndex?: number;
  targetBreakpointIndex?: number;
  errorCode?: string;
  errorMessage?: string;
}

export interface IWheelConfig extends IDeviceConfig {
  breakPoints: number[];
  stepsPerRevolution?: number;
  maxStepsPerRevolution?: number;
  zeroPointDegree?: number;
}

export function useWheel(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IWheelState, IWheelConfig>(deviceId);

  const calibrate = (maxStepsPerRevolution?: number) =>
    execDeviceFn(
      "calibrate",
      maxStepsPerRevolution !== undefined ? { maxStepsPerRevolution } : undefined
    );

  const init = (maxStepsPerRevolution?: number) =>
    execDeviceFn(
      "init",
      maxStepsPerRevolution !== undefined ? { maxStepsPerRevolution } : undefined
    );

  const nextBreakpoint = () => execDeviceFn("next-breakpoint", {});

  const moveToAngle = (angle: number) => execDeviceFn("move-to-angle", { angle });

  const stop = () => execDeviceFn("stop", {});

  return [
    device,
    {
      ...actions,
      calibrate,
      init,
      nextBreakpoint,
      moveToAngle,
      stop,
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
