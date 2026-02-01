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
  errorCode?: number;
  errorMessage?: string;
  [key: string]: unknown;
}

export interface IWheelConfig extends IDeviceConfig {
  breakPoints: number[];
  stepsPerRevolution?: number;
  maxStepsPerRevolution?: number;
  zeroPointDegree?: number;
  [key: string]: unknown;
}

export function useWheel(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IWheelState, IWheelConfig>(deviceId);

  const calibrate = (maxStepsPerRevolution?: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "calibrate",
      args:
        maxStepsPerRevolution !== undefined
          ? {
              maxStepsPerRevolution,
            }
          : undefined,
    });

  const init = (maxStepsPerRevolution?: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "init",
      args:
        maxStepsPerRevolution !== undefined
          ? {
              maxStepsPerRevolution,
            }
          : undefined,
    });

  const nextBreakpoint = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "next-breakpoint",
    });

  const moveToAngle = (angle: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "move-to-angle",
      args: {
        angle,
      },
    });

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "stop",
    });

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
