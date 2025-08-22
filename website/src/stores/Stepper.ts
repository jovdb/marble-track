import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

const deviceType = "stepper";
export interface IStepperState extends IDeviceState {
  steps: number;
  maxSpeed: number;
  maxAcceleration: number;
  currentPosition: number;
}

export function move(
  deviceId: string,
  args: { steps: number; maxSpeed?: number; maxAcceleration?: number }
) {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "move",
    args,
  } as IWsDeviceMessage);
}

export function stop(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType,
    deviceId,
    fn: "stop",
  } as IWsDeviceMessage);
}

export function createStepperStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    move: (args: Parameters<typeof move>[1]) => move(deviceId, args),
    stop: () => stop(deviceId),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IStepperState;
  }
}
