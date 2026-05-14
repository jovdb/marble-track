import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "servogate";

export interface IServoGateState extends IDeviceState {
  gateState: "Idle" | "WaitOpen" | "Opening" | "WaitClose" | "Closing" | "Between";
  queueCount: number;
  pulseCount: number;
  [key: string]: unknown;
}

export interface IServoGateConfig extends IDeviceConfig {
  name?: string;
  openDelayMs?: number;
  closeDelayMs?: number;
  betweenDelayMs?: number;
  fullQueueCount?: number;
  initialQueueCount?: number;
  [key: string]: unknown;
}

export function useServoGate(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IServoGateState, IServoGateConfig>(
    deviceId
  );

  const trigger = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "trigger",
    });

  const reset = () =>
    sendMessage({
      type: "device-fn",
      deviceId,
      deviceType,
      fn: "reset",
    });

  return [
    device,
    {
      ...actions,
      trigger,
      reset,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IServoGateState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IServoGateConfig;
  }
}
