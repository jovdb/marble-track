import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "buzzer";

interface IBuzzerState extends IDeviceState {
  playing?: boolean;
  currentTune?: string;
  mode?: string;
  [key: string]: unknown;
}

export interface IBuzzerConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  [key: string]: unknown;
}

export function useBuzzer(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IBuzzerState, IBuzzerConfig>(deviceId);

  const tone = (args: { frequency: number; duration: number }) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "tone",
      args,
    });

  const tune = (rtttl: string) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "tune",
      args: { rtttl },
    });

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "stop",
      args: {},
    });

  return [
    device,
    {
      ...actions,
      tone,
      tune,
      stop,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IBuzzerState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IBuzzerConfig;
  }
}
