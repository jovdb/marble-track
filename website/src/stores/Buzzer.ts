import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "buzzer";

interface IBuzzerState extends IDeviceState {
  playing?: boolean;
  currentTune?: string;
  mode?: string;
}

export interface IBuzzerConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
}

export function useBuzzer(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IBuzzerState, IBuzzerConfig>(deviceId);

  const tone = (args: { frequency: number; duration: number }) => execDeviceFn("tone", args);

  const tune = (rtttl: string) => execDeviceFn("tune", { rtttl });

  const stop = () => execDeviceFn("stop", {});

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
