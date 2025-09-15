import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage } from "../hooks/useWebSocket";
import { IWsDeviceMessage } from "../interfaces/WebSockets";

const deviceType = "button";

interface IButtonState extends IDeviceState {
  pressed: boolean;
}

export function pressButton(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "pressed",
  } as IWsDeviceMessage);
}

export function releaseButton(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "released",
  } as IWsDeviceMessage);
}

export function createButtonStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    press: () => pressButton(deviceId),
    release: () => releaseButton(deviceId),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IButtonState;
  }
}
