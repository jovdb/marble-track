import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

const deviceType = "led";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF";
}

export function setLed(deviceId: string, value: any) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "set",
    args: { value },
  } as IWsDeviceMessage);
}

export function createLedStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    setLed: (value: Parameters<typeof setLed>[1]) => setLed(deviceId, value),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILedState;
  }
}
