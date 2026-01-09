import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";
import { PinConfig } from "../interfaces/WebSockets";

const deviceType = "button";

interface IButtonState extends IDeviceState {
  isPressed: boolean;
  value: number;
  isPressedChanged: boolean;
  [key: string]: unknown;
}

export interface IButtonConfig extends IDeviceConfig {
  name?: string;
  pin?: PinConfig;
  pinMode?: "Floating" | "PullUp" | "PullDown";
  buttonType?: "NormalOpen" | "NormalClosed";
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
