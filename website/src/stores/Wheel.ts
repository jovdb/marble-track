import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "wheel";
export interface IWheelState extends IDeviceState {
  state: "CALIBRATING" | "IDLE" | "RESET" | "MOVING";
  lastZeroPosition: number;
  stepsInLastRevolution: number;
  angle: number | null;
  targetAngle?: number;
  speedRpm?: number;
  accelerationRpmSq?: number;
  [key: string]: unknown;
}

export interface IWheelConfig extends IDeviceConfig {
  breakPoints: number[];
  stepsPerRevolution?: number;
  maxStepsPerRevolution?: number;
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

  return [
    device,
    {
      ...actions,
      calibrate,
      reset,
      nextBreakpoint,
      moveToAngle,
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
