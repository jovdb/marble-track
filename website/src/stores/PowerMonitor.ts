import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "powermonitor";

export interface IPowerMonitorState extends IDeviceState {
  status?: string;
  voltage?: number; // V
  current?: number; // A
  watt?: number; // W
  timestamp?: number; // millis() at last reading
  foundAddresses?: number[];
}

export interface IPowerMonitorConfig extends IDeviceConfig {
  name?: string;
  i2cDeviceId?: string;
  i2cAddress?: number;
  shuntResistance?: number; // ohms
  maxCurrent?: number; // A
}

export function usePowerMonitor(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IPowerMonitorState, IPowerMonitorConfig>(
    deviceId
  );

  const init = () => execDeviceFn("init", {});
  const scan = () => execDeviceFn("scan", {});

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
    [deviceType]: IPowerMonitorState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IPowerMonitorConfig;
  }
}
