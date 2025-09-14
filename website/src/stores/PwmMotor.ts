import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

const deviceType = "pwmmotor";

interface IPwmMotorState extends IDeviceState {
  pin: number;
  pwmChannel: number;
  frequency: number;
  resolutionBits: number;
  dutyCycle: number;
  /** Available when animating */
  targetDutyCycle?: number;
  /** Available when animating */
  targetDurationMs?: number;

  running?: boolean;
}

export function setDutyCycle(deviceId: string, value: number, durationMs?: number) {
  const args: { value: number; durationMs?: number } = { value };

  if (durationMs !== undefined && durationMs > 0) {
    args.durationMs = durationMs;
  }

  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "setDutyCycle",
    args,
  } as IWsDeviceMessage);
}

export function stop(deviceId: string) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "stop",
    args: {},
  } as IWsDeviceMessage);
}

export function setupMotor(
  deviceId: string,
  pin: number,
  channel: number,
  frequency: number,
  resolutionBits: number
) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "setup",
    args: { pin, channel, frequency, resolutionBits },
  } as IWsDeviceMessage);
}

export function createPwmMotorStore(deviceId: string) {
  const base = createDeviceStore(deviceId, deviceType);

  return {
    ...base,
    setDutyCycle: (value: number, durationMs?: number) => setDutyCycle(deviceId, value, durationMs),
    stop: () => stop(deviceId),
    setupMotor: (pin: number, channel: number, frequency: number, resolutionBits: number) =>
      setupMotor(deviceId, pin, channel, frequency, resolutionBits),
  };
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IPwmMotorState;
  }
}
