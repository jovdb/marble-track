import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "pwm";

interface IPwmState extends IDeviceState {
  pin?: number;
  pwmChannel?: number;
  frequency?: number;
  resolutionBits?: number;
  dutyCycle?: number;
  targetDutyCycle?: number;
  targetDurationMs?: number;
  running?: boolean;
  [key: string]: unknown;
}

export interface IPwmConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  pwmChannel?: number;
  frequency?: number;
  resolutionBits?: number;
  [key: string]: unknown;
}

export function usePwm(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IPwmState, IPwmConfig>(deviceId);

  const setDutyCycle = (value: number, durationMs?: number) => {
    const args: { value: number; durationMs?: number } = { value };
    if (durationMs !== undefined && durationMs > 0) {
      args.durationMs = durationMs;
    }

    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setDutyCycle",
      args,
    });
  };

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "stop",
      args: {},
    });

  const setupMotor = (pin: number, channel: number, frequency: number, resolutionBits: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setup",
      args: { pin, channel, frequency, resolutionBits },
    });

  return [
    device,
    {
      ...actions,
      setDutyCycle,
      stop,
      setupMotor,
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
