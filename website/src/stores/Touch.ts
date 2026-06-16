import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "touch";

interface ITouchState extends IDeviceState {
  touched: boolean;
  value: number;
  isTouchedChanged: boolean;
}

export interface ITouchConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  threshold?: number;
  durationMs?: number;
}

export function useTouch(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<ITouchState, ITouchConfig>(deviceId);

  const touch = () => execDeviceFn("touch", {});

  const untouch = () => execDeviceFn("untouch", {});

  const setStreaming = (enabled: boolean, intervalMs = 500) =>
    execDeviceFn("setStreaming", { enabled, intervalMs });

  const readValue = () => actions.getDeviceState();

  return [
    device,
    {
      ...actions,
      touch,
      untouch,
      setStreaming,
      readValue,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ITouchState;
  }

  export interface IDeviceConfigs {
    [deviceType]: ITouchConfig;
  }
}
