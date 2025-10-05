import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "button";

interface IButtonState extends IDeviceState {
  pressed: boolean;
}

export interface IButtonConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  pinMode?: "floating" | "pullup" | "pulldown";
  debounceMs?: number;
  buttonType?: "NormalOpen" | "NormalClosed";
}

export function useButton(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IButtonState, IButtonConfig>(deviceId);

  const press = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "pressed",
      args: {},
    });

  const release = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "released",
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
