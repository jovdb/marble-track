import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";
import { IDeviceState } from "../components/devices/Device";

interface IBuzzerState extends IDeviceState {
  playing: boolean;
  currentTune: string;
  mode: string;
}

export function createBuzzerStore(deviceId: string) {
  const deviceType = "buzzer";
  const base = createDeviceStore<IBuzzerState, unknown>(deviceId);

  /** Play a tone */
  const tone = (args: { frequency: number; duration: number }) => {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "tone",
      args,
    } as IWsDeviceMessage);
  };

  /** Play a rtttl tune */
  const tune = (rtttl: string) => {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "tune",
      args: {
        rtttl,
      },
    } as IWsDeviceMessage);
  };

  return {
    ...base,
    tone,
    tune,
  };
}
