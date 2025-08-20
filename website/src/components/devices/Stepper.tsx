import { Device, IDeviceState } from "./Device";
import { createDeviceState, IWsDeviceMessage, sendMessage } from "../../hooks/useWebSocket";
import { createSignal } from "solid-js";
import styles from "./Device.module.css";

export interface IStepperState extends IDeviceState {
  steps: number;
  maxSpeed: number;
  maxAcceleration: number;
  currentPosition: number;
}

export function Stepper(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IStepperState>(props.id);
  const [steps, setSteps] = createSignal(deviceState()?.steps || 1000);
  const [maxSpeed, setMaxSpeed] = createSignal(deviceState()?.maxSpeed || 1000);
  const [maxAcceleration, setAcceleration] = createSignal(deviceState()?.maxAcceleration || 300);

  const updateStepper = () => {
    sendMessage({
      type: "device-fn",
      deviceId: props.id,
      fn: "move",
      steps: steps(),
      maxSpeed: maxSpeed(),
      maxAcceleration: maxAcceleration(),
    } as IWsDeviceMessage);
  };

  const stopStepper = () => {
    sendMessage({
      type: "device-fn",
      deviceId: props.id,
      fn: "stop",
    } as IWsDeviceMessage);
  };

  return (
    <Device id={props.id} deviceState={deviceState()}>
      {disabled() && (
        <div class={styles.device__error}>
          {error() || (connectedState() === "Disconnected" ? "Disconnected" : connectedState())}
        </div>
      )}
      {!disabled() && (
        <>
          {/*
          <div class={styles.device__status}>
            <span class={styles["device__status-text"]}>
              Steps: {steps() ?? 0}
            </span>
            <span class={styles["device__status-text"]}>
              Max Speed: {maxSpeed() ?? 0} steps/s
            </span>
            <span class={styles["device__status-text"]}>
              Max Acceleration: {maxAcceleration() ?? 0} steps/s²
            </span>
          </div>
          */}
          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`steps-${props.id}`}>
              Steps: {steps()} steps
            </label>
            <input
              id={`steps-${props.id}`}
              class={styles.device__input}
              type="range"
              min="-10000"
              max="10000"
              value={steps()}
              onInput={(e) => setSteps(Number(e.currentTarget.value))}
            />
          </div>
          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`maxSpeed-${props.id}`}>
              Max Speed: {maxSpeed()} steps/s
            </label>
            <input
              id={`maxSpeed-${props.id}`}
              class={styles.device__input}
              type="range"
              min="10"
              max="2000"
              value={maxSpeed()}
              onInput={(e) => setMaxSpeed(Number(e.currentTarget.value))}
            />
          </div>
          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`maxAcceleration-${props.id}`}>
              Max Acceleration: {maxAcceleration()} steps/s²
            </label>
            <input
              id={`maxAcceleration-${props.id}`}
              class={styles.device__input}
              type="range"
              min="10"
              max="2000"
              value={maxAcceleration()}
              onInput={(e) => setAcceleration(Number(e.currentTarget.value))}
            />
          </div>
          <div class={styles.device__controls}>
            <button class={styles.device__button} onClick={updateStepper} disabled={disabled()}>
              Move
            </button>
            <button class={styles.device__button} onClick={stopStepper} disabled={disabled()}>
              Stop
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
