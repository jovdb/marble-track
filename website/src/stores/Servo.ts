import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage } from "../hooks/useWebSocket";
import { IWsDeviceMessage } from "../interfaces/WebSockets";

interface IServoState extends IDeviceState {
  angle: number;
  targetAngle: number;
  speed: number;
  isMoving: boolean;
  pin: number;
  pwmChannel: number;
}

const deviceType = "servo";

const setAngle = (
  deviceId: string,
  args: {
    angle: number;
    speed?: number;
  }
) => {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "setAngle",
    args,
  } as IWsDeviceMessage);
};

const setSpeed = (deviceId: string, speed: number) => {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "setSpeed",
    args: { speed },
  } as IWsDeviceMessage);
};

const stop = (deviceId: string) => {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "stop",
  } as IWsDeviceMessage);
};

export function createServoStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    setAngle: (args: Parameters<typeof setAngle>[1]) => setAngle(deviceId, args),
    setSpeed: (speed: Parameters<typeof setSpeed>[1]) => setSpeed(deviceId, speed),
    stop: () => stop(deviceId),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IServoState;
  }
}
