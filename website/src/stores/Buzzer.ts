import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";
import { IDeviceState } from "../components/devices/Device";

interface IBuzzerState extends IDeviceState {
  playing: boolean;
  currentTune: string;
  mode: string;
}

export function playTone(deviceId: string, args: { frequency: number; duration: number }) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType: "buzzer",
    fn: "tone",
    args,
  } as IWsDeviceMessage);
}

export function playTune(deviceId: string, rtttl: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType: "buzzer",
    fn: "tune",
    args: { rtttl },
  } as IWsDeviceMessage);
}

export function createBuzzerStore(deviceId: string) {
  const base = createDeviceStore<IBuzzerState, unknown>(deviceId);

  return {
    ...base,
    tone: (args: Parameters<typeof playTone>[1]) => playTone(deviceId, args),
    tune: (rtttl: string) => playTune(deviceId, rtttl),
  };
}
