import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "i2c";

interface II2cState extends IDeviceState {
  [key: string]: unknown;
}

interface II2cConfig extends IDeviceConfig {
  name: string;
  sdaPin: number;
  sclPin: number;
  [key: string]: unknown;
}

export function useI2c(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<II2cState, II2cConfig>(deviceId);

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