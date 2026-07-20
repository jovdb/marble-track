import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "i2c";

export interface II2cState extends IDeviceState {
  foundAddresses: number[];
}

export interface II2cConfig extends IDeviceConfig {
  name: string;
  sdaPin: number;
  sclPin: number;
}

export function useI2c(deviceId: string | (() => string)) {
  const [device, { execDeviceFn, ...actions }] = useDevice<II2cState, II2cConfig>(deviceId);

  const scanBus = () => execDeviceFn("scan", undefined);

  return [
    device,
    {
      ...actions,
      scanBus,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: II2cState;
  }

  export interface IDeviceConfigs {
    [deviceType]: II2cConfig;
  }
}
