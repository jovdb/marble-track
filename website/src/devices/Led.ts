import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "../devices/Device";
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
      type: "device-action",
      deviceId,
      deviceType,
      fn: "set",
      value,
    } as IWsDeviceMessage);
  };

  return {
    ...base,
    setLed,
  };
}
