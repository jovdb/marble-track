import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

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
    deviceType: "stepper",
    deviceId,
    fn: "move",
    args,
  } as IWsDeviceMessage);
}

export function stop(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceType: "stepper",
    deviceId,
    fn: "stop",
  } as IWsDeviceMessage);
}

export function createStepperStore(deviceId: string) {
  const base = createDeviceStore<IStepperState, unknown>(deviceId);

  return {
    ...base,
    move: (args: Parameters<typeof move>[1]) => move(deviceId, args),
    stop: () => stop(deviceId),
  };
}
