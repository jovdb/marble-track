import { IDeviceConfig, IDeviceState } from "./Devices";
import { useDevice } from "./Devices";
import { PinConfig } from "../interfaces/WebSockets";

const deviceType = "hv20t";

export interface IHv20tAudioState extends IDeviceState {
  volumePercent?: number;
  currentPlayingSong?: number;
  songQueue?: number[];
}

export interface IHv20tAudioConfig extends IDeviceConfig {
  name?: string;
  rxPin?: PinConfig | number;
  txPin?: PinConfig | number;
  defaultVolumePercent?: number;
}

export type Hv20tPlayMode = "skip" | "stop" | "queue";

export function useHv20tAudio(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IHv20tAudioState, IHv20tAudioConfig>(
    deviceId
  );

  const play = (songIndex: number, mode: Hv20tPlayMode = "stop") =>
    execDeviceFn("play", { songIndex, mode });

  const stop = () => execDeviceFn("stop", {});

  const setVolume = (percent: number) => execDeviceFn("setVolume", { percent });

  return [
    device,
    {
      ...actions,
      play,
      stop,
      setVolume,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IHv20tAudioState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IHv20tAudioConfig;
  }
}
