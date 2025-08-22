import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";
import { IDeviceState } from "../components/devices/Device";

interface IButtonState extends IDeviceState {
  pressed: boolean;
}

export function pressButton(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType: "button",
    fn: "pressed",
  } as IWsDeviceMessage);
}

export function releaseButton(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType: "button",
    fn: "released",
  } as IWsDeviceMessage);
}

export function createButtonStore(deviceId: string) {
  const base = createDeviceStore<IButtonState, unknown>(deviceId);

  return {
    ...base,
    press: () => pressButton(deviceId),
    release: () => releaseButton(deviceId),
  };
}
