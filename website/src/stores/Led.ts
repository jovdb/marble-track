import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF";
}

export function setLed(deviceId: string, value: any) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType: "led",
    fn: "set",
    args: { value },
  } as IWsDeviceMessage);
}

export function createLedStore(deviceId: string) {
  const base = createDeviceStore<ILedState, unknown>(deviceId);

  return {
    ...base,
    setLed: (value: Parameters<typeof setLed>[1]) => setLed(deviceId, value),
  };
}
