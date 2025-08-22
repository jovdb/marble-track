import { IDeviceState } from "../components/devices/Device";
import { createDeviceStore } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

export interface IStepperState extends IDeviceState {
  steps: number;
  maxSpeed: number;
  maxAcceleration: number;
  currentPosition: number;
}

export function createStepperStore(deviceId: string) {
  const deviceType = "stepper";
  const base = createDeviceStore<IStepperState, unknown>(deviceId);

  // Set stepper state (e.g., step, direction, speed, etc.)
  const move = (args: { steps: number; maxSpeed?: number; maxAcceleration?: number }) => {
    sendMessage({
      type: "device-fn",
      deviceType,
      deviceId,
      fn: "move",
      args,
    } as IWsDeviceMessage);
  };

  const stop = () => {
    sendMessage({
      type: "device-fn",
      deviceType,
      deviceId,
      fn: "stop",
    } as IWsDeviceMessage);
  };

  return {
    ...base,
    move,
    stop,
  };
}
