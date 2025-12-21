import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "button";

interface IButtonState extends IDeviceState {
  isPressed: boolean;
  [key: string]: unknown;
}

export interface IButtonConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  pinMode?: "Floating" | "PullUp" | "PullDown";
  debounceTimeInMs?: number;
  [key: string]: unknown;
}

export function useButton(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IButtonState, IButtonConfig>(deviceId);

  const press = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "press",
      args: {},
    });

  const release = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "release",
      args: {},
    });

  return [
    device,
    {
      ...actions,
      press,
      release,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IButtonState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IButtonConfig;
  }
}
