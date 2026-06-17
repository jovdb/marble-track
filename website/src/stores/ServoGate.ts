import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "servogate";

export interface IServoGateState extends IDeviceState {
  gateState: "Idle" | "WaitOpen" | "Opening" | "WaitClose" | "Closing" | "Between";
  queueCount: number;
}

export interface IServoGateConfig extends IDeviceConfig {
  name?: string;
  openDelayMs?: number;
  closeDelayMs?: number;
  betweenDelayMs?: number;
  fullQueueCount?: number;
  initialQueueCount?: number;
}

export function useServoGate(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IServoGateState, IServoGateConfig>(
    deviceId
  );

  const trigger = () => execDeviceFn("trigger", undefined);

  const reset = () => execDeviceFn("reset", undefined);

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
