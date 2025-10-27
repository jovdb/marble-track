import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "lift";
export interface ILiftState extends IDeviceState {
  state: "IDLE" | "MOVING";
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

  return [
    device,
    {
      ...actions,
      up,
      down,
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
