import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "i2c";

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface II2cState extends IDeviceState {}

export interface II2cConfig extends IDeviceConfig {
  name: string;
  sdaPin: number;
  sclPin: number;
}

export function useI2c(deviceId: string) {
  const [device, { ...actions }] = useDevice<II2cState, II2cConfig>(deviceId);

  return [
    device,
    {
      ...actions,
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
