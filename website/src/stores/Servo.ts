import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "servo";

interface IServoState extends IDeviceState {
  value?: number;
  targetValue?: number;
  targetDurationMs?: number;
  running?: boolean;
  [key: string]: unknown;
}

export interface IServoConfig extends IDeviceConfig {
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

export function useServo(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IServoState, IServoConfig>(deviceId);

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

  return [
    device,
    {
      ...actions,
      setValue,
      stop,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IServoState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IServoConfig;
  }
}
