import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "pwmmotor";

interface IPwmMotorState extends IDeviceState {
  value?: number;
  targetValue?: number;
  targetDurationMs?: number;
  running?: boolean;
  [key: string]: unknown;
}

export interface IPwmMotorConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  mcpwmChannel?: number;
  frequency?: number;
  resolutionBits?: number;
  minDutyCycle?: number;
  maxDutyCycle?: number;
  defaultDurationInMs?: number;
  [key: string]: unknown;
}

export function usePwmMotor(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IPwmMotorState, IPwmMotorConfig>(
    deviceId
  );

  const setValue = (value: number, durationMs?: number) => {
    const args: { value: number; durationMs?: number } = { value };
    if (durationMs !== undefined && durationMs > 0) {
      args.durationMs = durationMs;
    }

    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setValue",
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

  const setupMotor = (
    pin: number,
    mcpwmChannel: number,
    frequency: number,
    resolutionBits: number
  ) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setup",
      args: { pin, mcpwmChannel, frequency, resolutionBits },
    });

  return [
    device,
    {
      ...actions,
      setValue,
      stop,
      setupMotor,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IPwmMotorState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IPwmMotorConfig;
  }
}
