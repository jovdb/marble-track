import { createEffect, createMemo, createSignal } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import { useStepper } from "../../stores/Stepper";
import StepperConfig from "./StepperConfig";
import { StepperIcon } from "../icons/Icons";

export function Stepper(props: { id: string }) {
  const stepperStore = useStepper(props.id);
  const device = () => stepperStore[0];
  const actions = stepperStore[1];

  const state = createMemo(() => device()?.state);
  const config = createMemo(() => device()?.config);
  const isMoving = createMemo(() => Boolean(state()?.isMoving));
  const currentPosition = createMemo(() => state()?.currentPosition ?? 0);
  const targetPosition = createMemo(() => state()?.targetPosition ?? 0);

  const [steps, setSteps] = createSignal(1000);
  const [maxSpeed, setMaxSpeed] = createSignal(1000);
  const [maxAcceleration, setMaxAcceleration] = createSignal(300);

  createEffect(() => {
    const next = state()?.maxSpeed;
    if (typeof next === "number") {
      setMaxSpeed(next);
    }
  });

  createEffect(() => {
    const fallback = config()?.maxSpeed;
    if (typeof fallback === "number") {
      setMaxSpeed(fallback);
    }
  });

  createEffect(() => {
    const s = state();
    const next =
      typeof s?.maxAcceleration === "number"
        ? s.maxAcceleration
        : (s as { acceleration?: number } | undefined)?.acceleration;
    if (typeof next === "number") {
      setMaxAcceleration(next);
    }
  });

  createEffect(() => {
    const cfg = config();
    const fallback =
      typeof cfg?.maxAcceleration === "number"
        ? cfg.maxAcceleration
        : (cfg as { acceleration?: number } | undefined)?.acceleration;
    if (typeof fallback === "number") {
      setMaxAcceleration(fallback);
    }
  });

  const handleMove = () =>
    actions.move({
      steps: Math.trunc(steps()),
      maxSpeed: maxSpeed(),
      maxAcceleration: Math.trunc(maxAcceleration()),
    });

  const handleStop = () => actions.stop();

  return (
    <Device
      id={props.id}
      deviceState={device()?.state}
      configComponent={(onClose) => <StepperConfig id={props.id} onClose={onClose} />}
      icon={<StepperIcon />}
    >
      <div class={styles.device__status}>
        <span class={styles["device__status-text"]}>Status: {isMoving() ? "Moving" : "Idle"}</span>
        <span class={styles["device__status-text"]}>Position: {currentPosition()}</span>
        <span class={styles["device__status-text"]}>Target: {targetPosition()}</span>
      </div>
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
          onInput={(event) => setSteps(Number(event.currentTarget.value))}
        />
      </div>
      <div class={styles["device__input-group"]}>
        <label class={styles.device__label} for={`maxSpeed-${props.id}`}>
          Speed: {maxSpeed()} steps/s
        </label>
        <input
          id={`maxSpeed-${props.id}`}
          class={styles.device__input}
          type="range"
          min="10"
          max="2000"
          value={maxSpeed()}
          onInput={(event) => setMaxSpeed(Number(event.currentTarget.value))}
        />
      </div>
      <div class={styles["device__input-group"]}>
        <label class={styles.device__label} for={`maxAcceleration-${props.id}`}>
          Acceleration: {maxAcceleration()} steps/s²
        </label>
        <input
          id={`maxAcceleration-${props.id}`}
          class={styles.device__input}
          type="range"
          min="10"
          max="2000"
          value={maxAcceleration()}
          onInput={(event) => setMaxAcceleration(Number(event.currentTarget.value))}
        />
      </div>
      <div class={styles.device__controls}>
        <button class={styles.device__button} onClick={handleMove}>
          Move
        </button>
        <button class={styles.device__button} onClick={handleStop}>
          Stop
        </button>
      </div>
    </Device>
  );
}
