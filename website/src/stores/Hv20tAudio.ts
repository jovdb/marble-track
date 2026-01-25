import { IDeviceConfig, IDeviceState } from "./Devices";
import { useDevice } from "./Devices";
import { PinConfig } from "../interfaces/WebSockets";

const deviceType = "hv20t";

export interface IHv20tAudioState extends IDeviceState {
  isBusy?: boolean;
  volumePercent?: number;
  lastSongIndex?: number;
  [key: string]: unknown;
}

export interface IHv20tAudioConfig extends IDeviceConfig {
  name?: string;
  rxPin?: PinConfig | number;
  txPin?: PinConfig | number;
  busyPin?: PinConfig | number;
  defaultVolumePercent?: number;
  [key: string]: unknown;
}

export type Hv20tPlayMode = "skip" | "stop" | "queue";

export function useHv20tAudio(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IHv20tAudioState, IHv20tAudioConfig>(
    deviceId
  );

  const play = (songIndex: number, mode: Hv20tPlayMode = "stop") =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "play",
      args: { songIndex, mode },
    });

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "stop",
      args: {},
    });

  const setVolume = (percent: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setVolume",
      args: { percent },
    });

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
