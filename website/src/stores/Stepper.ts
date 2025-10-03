import { IDeviceConfig, IDeviceState } from "./Device";
import { useDevice } from "./Devices";

const deviceType = "stepper";

export const STEPPER_TYPES = ["DRIVER", "HALF4WIRE"] as const;
export type StepperType = (typeof STEPPER_TYPES)[number];

export interface IStepperState extends IDeviceState {
  currentPosition?: number;
  targetPosition?: number;
  isMoving?: boolean;
  maxSpeed?: number;
  maxAcceleration?: number;
  stepperType?: StepperType;
  is4Pin?: boolean;
  configured?: boolean;
}

export interface IStepperDriverPins {
  stepPin: number;
  dirPin: number;
}

export type IStepperFourWirePins = [number, number, number, number];

export type IStepperPins = IStepperDriverPins | IStepperFourWirePins;

export interface IStepperConfig extends IDeviceConfig {
  name?: string;
  configured?: boolean;
  stepperType?: StepperType;
  is4Pin?: boolean;
  maxSpeed?: number;
  maxAcceleration?: number;
  pins?: IStepperPins;
}

export interface IStepperMoveArgs {
  steps: number;
  speed?: number;
  acceleration?: number;
}

export function useStepper(deviceId: string) {
  const [device, { sendMessage, ...actions }] = useDevice<IStepperState, IStepperConfig>(deviceId);

  const move = (args: IStepperMoveArgs) =>
    sendMessage({
      type: "device-fn",
      deviceType,
      deviceId,
      fn: "move",
      args,
    });

  const stop = () =>
    sendMessage({
      type: "device-fn",
      deviceType,
      deviceId,
      fn: "stop",
      args: {},
    });

  const resetPosition = () =>
    sendMessage({
      type: "device-fn",
      deviceType,
      deviceId,
      fn: "setCurrentPosition",
      args: { position: 0 },
    });

  return [
    device,
    {
      ...actions,
      move,
      stop,
      resetPosition,
    },
  ] as const;
}

declare global {
  export interface IDeviceStates {
    [deviceType]: IStepperState;
  }

  export interface IDeviceConfigs {
    [deviceType]: IStepperConfig;
  }
}
