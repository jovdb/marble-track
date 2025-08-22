import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF";
}

export function createLedStore(deviceId: string) {
  const deviceType = "led";
  const base = createDeviceStore<ILedState, unknown>(deviceId);

  /** Set LED state (on/off, brightness, etc.) */
  const setLed = (value: any) => {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "set",
      args: { value },
    } as IWsDeviceMessage);
  };

  return {
    ...base,
    setLed,
  };
}
