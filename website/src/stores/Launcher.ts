import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "launcher";

export interface ILauncherState extends IDeviceState {
  state: "Init" | "Up" | "MovingUp" | "Down" | "MovingDown";
  isBallLoaded: boolean;
  isBallWaiting: boolean;
  [key: string]: unknown;
}

export interface ILauncherConfig extends IDeviceConfig {
  name?: string;
  loadTimeMs?: number;
  launchTimeMs?: number;
  [key: string]: unknown;
}

export function useLauncher(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<ILauncherState, ILauncherConfig>(
    deviceId
  );

  const init = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "init",
    });

  const load = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "load",
    });

  const launch = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "launch",
    });

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
