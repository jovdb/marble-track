import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "led";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF" | "BLINKING" | "PATTERN";
  blinkOnTime: number;
  blinkOffTime: number;
  blinkDelay: number;
  pattern?: number[];
}

export const LED_INITIAL_STATES = ["OFF", "ON", "BLINKING"] as const;
export type LedInitialState = (typeof LED_INITIAL_STATES)[number];

import { PinConfig } from "../interfaces/WebSockets";

interface ILedConfig extends IDeviceConfig {
  name: string;
  pin: PinConfig;
  initialState?: LedInitialState;
}

export function useLed(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<ILedState, ILedConfig>(deviceId);

  function setLed(_deviceId: string, value: any) {
    execDeviceFn("set", { value });
  }

  function blink(_deviceId: string, onTime?: number, offTime?: number) {
    execDeviceFn("blink", { onTime, offTime });
  }

  function pattern(_deviceId: string, timings: number[], synced = false) {
    execDeviceFn("pattern", { pattern: timings, synced });
  }

  return [
    device,
    {
      ...actions,
      setLed: (value: Parameters<typeof setLed>[1]) => setLed(deviceId, value),
      blink: (onTime?: number, offTime?: number) => blink(deviceId, onTime, offTime),
      pattern: (timings: number[], synced?: boolean) => pattern(deviceId, timings, synced),
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: ILedState;
  }

  export interface IDeviceConfigs {
    [deviceType]: ILedConfig;
  }
}
