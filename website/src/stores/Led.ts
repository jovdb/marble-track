import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage } from "../hooks/useWebSocket";
import { IWsDeviceMessage } from "../interfaces/WebSockets";

const deviceType = "led";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF" | "BLINKING";
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

export function blink(deviceId: string, onTime?: number, offTime?: number) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "blink",
    args: { onTime, offTime },
  } as IWsDeviceMessage);
}

export function createLedStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    setLed: (value: Parameters<typeof setLed>[1]) => setLed(deviceId, value),
    blink: (onTime?: number, offTime?: number) => blink(deviceId, onTime, offTime),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILedState;
  }
}
