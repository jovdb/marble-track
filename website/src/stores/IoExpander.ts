import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "ioexpander";

export type IoExpanderStateEnum = "Ready" | "Init" | "Error";

export interface IIoExpanderState extends IDeviceState {
  state?: IoExpanderStateEnum;
}

export interface IIoExpanderConfig extends IDeviceConfig {
  name?: string;
  expanderType?: string;
  i2cAddress?: number;
  i2cDeviceId?: string;
}

export function useIoExpander(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IIoExpanderState, IIoExpanderConfig>(
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
    [deviceType]: IIoExpanderState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IIoExpanderConfig;
  }
}
