import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "pwm";

interface IPwmState extends IDeviceState {
  dutyCycle: number;
  frequency: number;
  resolution: number;
  [key: string]: unknown;
}

interface IPwmConfig extends IDeviceConfig {
  name: string;
  pin: number;
  frequency?: number;
  resolution?: number;
  channel?: number;
  [key: string]: unknown;
}

export function usePwm(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IPwmState, IPwmConfig>(deviceId);

  function setDutyCycle(deviceId: string, dutyCycle: number) {
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "set-duty-cycle",
      args: { dutyCycle },
    });
  }

  return [
    device,
    {
      ...actions,
      setDutyCycle: (dutyCycle: number) => setDutyCycle(deviceId, dutyCycle),
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IPwmState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IPwmConfig;
  }
}