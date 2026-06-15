import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "powermonitor";

export interface IPowerMonitorState extends IDeviceState {
  status?: string;
  voltage?: number;   // V
  current?: number;   // A
  watt?: number;      // W
  timestamp?: number; // millis() at last reading
}

export interface IPowerMonitorConfig extends IDeviceConfig {
  name?: string;
  i2cDeviceId?: string;
  i2cAddress?: number;
  shuntResistance?: number; // ohms
  maxCurrent?: number;      // A
  minVoltage?: number;      // V (0% / alert threshold)
  maxVoltage?: number;      // V (100% reference)
  notifyIntervalMs?: number;
}

export function usePowerMonitor(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IPowerMonitorState, IPowerMonitorConfig>(
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
    [deviceType]: IPowerMonitorState;
  }
  export interface IDeviceConfigs {
    [deviceType]: IPowerMonitorConfig;
  }
}
