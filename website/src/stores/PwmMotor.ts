import { createDeviceStore, IDeviceState } from "./Device";
import { sendMessage, IWsDeviceMessage } from "../hooks/useWebSocket";

const deviceType = "pwmmotor";

interface IPwmMotorState extends IDeviceState {
  pin: number;
  pwmChannel: number;
  frequency: number;
  resolutionBits: number;
  dutyCycle: number;
  isSetup: boolean;
  running: boolean;
}

export function setDutyCycle(deviceId: string, value: number) {
  sendMessage({
    type: "device-fn",
    deviceId,
    deviceType,
    fn: "setDutyCycle",
    args: { value },
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
    setDutyCycle: (value: number) => setDutyCycle(deviceId, value),
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
