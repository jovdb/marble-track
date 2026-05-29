import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "pwmexpander";

export type PwmExpanderStateEnum = "Ready" | "Init" | "Error";

export interface IPwmExpanderState extends IDeviceState {
  state?: PwmExpanderStateEnum;
}

export interface IPwmExpanderConfig extends IDeviceConfig {
  name?: string;
  i2cDeviceId?: string;
  i2cAddress?: number;
  frequency?: number;
}

export function usePwmExpander(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IPwmExpanderState, IPwmExpanderConfig>(
    deviceId
  );

  const init = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "init",
    });

  return [
    device,
    {
      ...actions,
      init,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IPwmExpanderState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IPwmExpanderConfig;
  }
}
