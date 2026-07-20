import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "pwmexpander";

export type PwmExpanderStateEnum = "Ready" | "Init" | "Error";

export interface IPwmExpanderState extends IDeviceState {
  state?: PwmExpanderStateEnum;
  foundAddresses: number[];
}

export interface IPwmExpanderConfig extends IDeviceConfig {
  name?: string;
  i2cDeviceId?: string;
  i2cAddress?: number;
  frequency?: number;
}

export function usePwmExpander(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IPwmExpanderState, IPwmExpanderConfig>(
    deviceId
  );

  const init = () => execDeviceFn("init", undefined);
  const scan = () => execDeviceFn("scan", undefined);

  return [
    device,
    {
      ...actions,
      init,
      scan,
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
