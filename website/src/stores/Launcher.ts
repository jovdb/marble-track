import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "launcher";

export interface ILauncherState extends IDeviceState {
  state: "Unknown" | "Error" | "Up" | "MovingUp" | "Down" | "MovingDown";
  isBallLoaded: boolean;
  isBallWaiting: boolean;
}

export interface ILauncherConfig extends IDeviceConfig {
  name?: string;
  loadTimeMs?: number;
  launchTimeMs?: number;
}

export function useLauncher(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<ILauncherState, ILauncherConfig>(
    deviceId
  );

  const init = () => execDeviceFn("init", undefined);

  const load = () => execDeviceFn("load", undefined);

  const launch = () => execDeviceFn("launch", undefined);

  return [
    device,
    {
      ...actions,
      init,
      load,
      launch,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILauncherState;
  }

  export interface IDeviceConfigs {
    [deviceType]: ILauncherConfig;
  }
}
