import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "adxl345";

export interface IAdxl345State extends IDeviceState {
  status?: string;
  x?: number;
  y?: number;
  z?: number;
  roll?: number;
  pitch?: number;
  lastUpdated?: number;
}

export interface IAdxl345Config extends IDeviceConfig {
  name?: string;
  i2cDeviceId?: string;
  i2cAddress?: number;
  range?: number;
  refreshIntervalMs?: number;
  offsetX?: number;
  offsetY?: number;
  offsetZ?: number;
}

export function useAdxl345(deviceId: string) {
  const [state, { execDeviceFn, ...actions }] = useDevice<IAdxl345State, IAdxl345Config>(deviceId);

  const refresh = () => execDeviceFn("refresh", {});
  const calibrate = () => execDeviceFn("calibrate", {});

  return [
    state,
    {
      refresh,
      calibrate,
      ...actions,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IAdxl345State;
  }
  export interface IDeviceConfigs {
    [deviceType]: IAdxl345Config;
  }
}
