import { createEffect, createMemo, createSignal } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import stepperStyles from "./Stepper.module.css";
import { useStepper } from "../../stores/Stepper";
import StepperConfig from "./StepperConfig";
import { StepperIcon } from "../icons/Icons";

export function Stepper(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const stepperStore = useStepper(props.id);
  const device = () => stepperStore[0];
  const actions = stepperStore[1];

  const state = createMemo(() => device()?.state);
  const config = createMemo(() => device()?.config);
  const isMoving = createMemo(() => Boolean(state()?.isMoving));
  const currentPosition = createMemo(() => state()?.currentPosition ?? 0);
  const targetPosition = createMemo(() => state()?.targetPosition ?? 0);

  const [steps, setSteps] = createSignal(1000);
  const [speed, setSpeed] = createSignal(config()?.defaultSpeed ?? config()?.maxSpeed ?? 600);
  const [acceleration, setAcceleration] = createSignal(
    config()?.defaultAcceleration ?? config()?.maxAcceleration ?? 300
  );
  /*
  createEffect(() => {
    const next = state()?.maxSpeed;
    if (typeof next === "number") {
      setMaxSpeed(next);
    }
  });
  */

  createEffect(() => {
    const cfg = config();
    setSpeed(cfg?.defaultSpeed ?? cfg?.maxSpeed ?? 600);
  });

  createEffect(() => {
    const cfg = config();
    setAcceleration(cfg?.defaultAcceleration ?? cfg?.maxAcceleration ?? 300);
  });

  const handleMove = () =>
    actions.move({
      steps: Math.trunc(steps()),
      speed: speed(),
      acceleration: Math.trunc(acceleration()),
    });

  const handleStop = () => actions.stop(Math.trunc(acceleration()));

  const handleReset = () => actions.resetPosition();

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <StepperConfig id={props.id} onClose={onClose} />}
      icon={<StepperIcon />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
      stateComponent={() => null} 
    >
      <div class={styles.device__status}>
        <div
          classList={{
            [stepperStyles["stepper__status-indicator"]]: true,
            [stepperStyles["stepper__status-indicator--moving"]]: isMoving(),
            [stepperStyles["stepper__status-indicator--off"]]: !isMoving(),
          }}
        ></div>
        <span class={styles["device__status-text"]}>Position: {currentPosition()}</span>
        <span class={styles["device__status-text"]}>Target: {targetPosition()}</span>
      </div>
      <div class={styles["device__input-group"]}>
        <label class={styles.device__label} for={`steps-${props.id}`}>
          Steps:
        </label>
        <input
          id={`steps-${props.id}`}
          class={styles.device__input}
          type="number"
          value={steps()}
          onInput={(event) => setSteps(Number(event.currentTarget.value))}
          style={{ width: "8em" }}
        />
      </div>
      <div class={styles["device__input-group"]}>
        <label class={styles.device__label} for={`maxSpeed-${props.id}`}>
          Speed: {speed()} steps/s
        </label>
        <input
          id={`speed-${props.id}`}
          class={styles.device__input}
          type="range"
          min="1"
          max={config()?.maxSpeed ?? 1000}
          value={speed()}
          onInput={(event) => setSpeed(Number(event.currentTarget.value))}
        />
      </div>
      <div class={styles["device__input-group"]}>
        <label class={styles.device__label} for={`maxAcceleration-${props.id}`}>
          Acceleration: {acceleration()} steps/s²
        </label>
        <input
          id={`acceleration-${props.id}`}
          class={styles.device__input}
          type="range"
          min="1"
          max={config()?.maxAcceleration ?? 300}
          value={acceleration()}
          onInput={(event) => setAcceleration(Number(event.currentTarget.value))}
        />
      </div>
      <div class={styles.device__controls}>
        <button class={styles.device__button} onClick={handleMove}>
          Move
        </button>
        <button class={styles.device__button} onClick={handleStop}>
          Stop
        </button>
        <button class={styles.device__button} onClick={handleReset}>
          Reset Position
        </button>
      </div>
    </Device>
  );
}
