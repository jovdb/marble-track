import { IDeviceConfig, IDeviceState } from "./Device";
import { IDevice, useDevice } from "./Devices";

const deviceType = "led";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF" | "BLINKING";
}

interface ILedConfig extends IDeviceConfig {
  name: string;
  pin: number;
}

export function useLed(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<ILedState, ILedConfig>(deviceId);

  function setLed(deviceId: string, value: any) {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "set",
      args: { value },
    });
  }

  function blink(deviceId: string, onTime?: number, offTime?: number) {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "blink",
      args: { onTime, offTime },
    });
  }

  return [
    device,
    {
      ...actions,
      setLed: (value: Parameters<typeof setLed>[1]) => setLed(deviceId, value),
      blink: (onTime?: number, offTime?: number) => blink(deviceId, onTime, offTime),
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILedState;
  }
}
