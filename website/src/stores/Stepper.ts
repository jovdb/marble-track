import { IDeviceConfig, IDeviceState } from "./Device";
import { PinConfig } from "../interfaces/WebSockets";
import { useDevice } from "./Devices";

const deviceType = "stepper";

export const STEPPER_TYPES = ["DRIVER", "HALF4WIRE", "FULL4WIRE"] as const;
export type StepperType = (typeof STEPPER_TYPES)[number];

export interface IStepperState extends IDeviceState {
  currentPosition?: number;
  targetPosition?: number;
  isMoving?: boolean;
}

export interface IStepperConfig extends IDeviceConfig {
  name?: string;
  configured?: boolean;
  stepperType?: StepperType;
  usePwm?: boolean;
  maxSpeed?: number;
  maxAcceleration?: number;
  defaultSpeed?: number;
  defaultAcceleration?: number;
  // For DRIVER type
  stepPin?: PinConfig | number;
  dirPin?: PinConfig | number;
  // For 4-wire types
  pin1?: PinConfig | number;
  pin2?: PinConfig | number;
  pin3?: PinConfig | number;
  pin4?: PinConfig | number;
  // Common
  enablePin?: PinConfig | number;
  invertEnable?: boolean;
  invertDirection?: boolean;
}

export interface IStepperMoveArgs {
  steps: number;
  speed?: number;
  acceleration?: number;
}

export interface IStepperMoveToArgs {
  position: number;
  speed?: number;
  acceleration?: number;
}

export interface IStepperSetPositionArgs {
  position: number;
}

export function useStepper(deviceId: string) {
  const [device, { execDeviceFn, ...actions }] = useDevice<IStepperState, IStepperConfig>(deviceId);

  const move = (args: IStepperMoveArgs) => {
    const config = device?.config;
    const moveArgs = { ...args };

    // Use default values from config if not provided
    if (moveArgs.speed === undefined && config?.defaultSpeed !== undefined) {
      moveArgs.speed = config.defaultSpeed;
    }
    if (moveArgs.acceleration === undefined && config?.defaultAcceleration !== undefined) {
      moveArgs.acceleration = config.defaultAcceleration;
    }

    return execDeviceFn("move", moveArgs as unknown as Record<string, unknown>);
  };

  const stop = (acceleration?: number) => {
    const stopArgs: { acceleration?: number } = {};

    // Use provided acceleration, or fall back to default from config
    if (acceleration !== undefined) {
      stopArgs.acceleration = acceleration;
    }

    return execDeviceFn("stop", stopArgs);
  };

  const moveTo = (args: IStepperMoveToArgs) => {
    const config = device?.config;
    const moveArgs = { ...args };

    // Use default values from config if not provided
    if (moveArgs.speed === undefined && config?.defaultSpeed !== undefined) {
      moveArgs.speed = config.defaultSpeed;
    }
    if (moveArgs.acceleration === undefined && config?.defaultAcceleration !== undefined) {
      moveArgs.acceleration = config.defaultAcceleration;
    }

    return execDeviceFn("moveTo", moveArgs as unknown as Record<string, unknown>);
  };

  const setCurrentPosition = (args: IStepperSetPositionArgs) => {
    return execDeviceFn("setCurrentPosition", args as unknown as Record<string, unknown>);
  };

  const resetPosition = () => {
    return setCurrentPosition({ position: 0 });
  };

  return [
    device,
    {
      ...actions,
      move,
      moveTo,
      stop,
      setCurrentPosition,
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
