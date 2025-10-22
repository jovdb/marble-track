import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

interface IServoState extends IDeviceState {
  angle: number;
  targetAngle: number;
  speed: number;
  isMoving: boolean;
  pin: number;
  pwmChannel: number;
  [key: string]: unknown;
}

interface IServoConfig extends IDeviceConfig {
  name?: string;
  pin?: number;
  pwmChannel?: number;
  [key: string]: unknown;
}

const deviceType = "servo";

export function useServo(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IServoState, IServoConfig>(deviceId);

  const setAngle = (args: { angle: number; speed?: number }) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setAngle",
      args,
    });

  const setSpeed = (speed: number) =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "setSpeed",
      args: { speed },
    });

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "stop",
    });

  return [
    device,
    {
      ...actions,
      setAngle,
      setSpeed,
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
