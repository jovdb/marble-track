import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "servo";

export type ServoStateEnum = "Unknown" | "Disabled" | "Ready" | "Moving" | "Error";
export type ServoErrorCode = "" | "SetupFailed";

interface IServoState extends IDeviceState {
  state?: ServoStateEnum;
  value?: number;
  targetValue?: number;
  targetDurationMs?: number;
  running?: boolean;
  errorCode?: ServoErrorCode;
  errorMessage?: string;
}

export interface IServoConfig extends IDeviceConfig {
  name?: string;
  pin?: number | { pin: number; expanderId: string };
  mcpwmChannel?: number;
  frequency?: number;
  resolutionBits?: number;
  minDutyCycle?: number;
  maxDutyCycle?: number;
  defaultDurationInMs?: number;
}

export function useServo(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IServoState, IServoConfig>(deviceId);

  const setValue = (value: number, durationMs?: number) => {
    const args: { value: number; durationMs?: number } = { value };
    if (durationMs !== undefined) {
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

  const disable = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "disable",
      args: {},
    });

  return [
    device,
    {
      ...actions,
      setValue,
      stop,
      disable,
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
