import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

const deviceType = "buzzer";

interface IBuzzerState extends IDeviceState {
  playing: boolean;
  currentTune: string;
  mode: string;
}

export function playTone(deviceId: string, args: { frequency: number; duration: number }) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "tone",
    args,
  } as IWsDeviceMessage);
}

export function playTune(deviceId: string, rtttl: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "tune",
    args: { rtttl },
  } as IWsDeviceMessage);
}

export function createBuzzerStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    tone: (args: Parameters<typeof playTone>[1]) => playTone(deviceId, args),
    tune: (rtttl: string) => playTune(deviceId, rtttl),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IBuzzerState;
  }
}
