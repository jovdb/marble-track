import { createDeviceStore } from "../devices/Device";
import { sendMessage, IWsMessage } from "../hooks/useWebSocket";

export function createLedStore(deviceId: string) {
  const base = createDeviceStore(deviceId);

  // Set LED state (on/off, brightness, etc.)
  const setLed = (value: any) => {
    sendMessage({
      type: "device-action",
      deviceId,
      fn: "setLed",
      value,
    } as IWsMessage);
  };

  return {
    ...base,
    setLed,
  };
}
