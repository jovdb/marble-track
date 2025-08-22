import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";
import { IDeviceState } from "../components/devices/Device";

interface IButtonState extends IDeviceState {
  pressed: boolean;
}

export function createButtonStore(deviceId: string) {
  const deviceType = "button";
  const base = createDeviceStore<IButtonState, unknown>(deviceId);

  /** Simulate a button press */
  const press = () => {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "pressed",
    } as IWsDeviceMessage);
  };

  /** Simulate a button release */
  const release = () => {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "released",
    } as IWsDeviceMessage);
  };

  return {
    ...base,
    press,
    release,
  };
}
