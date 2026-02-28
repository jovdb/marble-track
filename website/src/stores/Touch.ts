import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "touch";

interface ITouchState extends IDeviceState {
  touched: boolean;
  value: number;
  isTouchedChanged: boolean;
  [key: string]: unknown;
}

export interface ITouchConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  threshold?: number;
  durationMs?: number;
  [key: string]: unknown;
}

export function useTouch(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<ITouchState, ITouchConfig>(deviceId);

  const touch = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "touch",
      args: {},
    });

  const untouch = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "untouch",
      args: {},
    });

  return [
    device,
    {
      ...actions,
      touch,
      untouch,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ITouchState;
  }

  export interface IDeviceConfigs {
    [deviceType]: ITouchConfig;
  }
}
