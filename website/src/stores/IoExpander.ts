import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "ioexpander";

export type IoExpanderStateEnum = "Ready" | "Init" | "Error";

export interface IIoExpanderState extends IDeviceState {
  state?: IoExpanderStateEnum;
  foundAddresses: number[];
}

export interface IIoExpanderConfig extends IDeviceConfig {
  name?: string;
  expanderType?: string;
  i2cAddress?: number;
  i2cDeviceId?: string;
}

export function useIoExpander(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IIoExpanderState, IIoExpanderConfig>(
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
    [deviceType]: IIoExpanderState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IIoExpanderConfig;
  }
}
