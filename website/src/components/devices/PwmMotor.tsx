import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";
import styles from "./Device.module.css";
import { createPwmMotorStore } from "../../stores/PwmMotor";
import { StepperIcon } from "../icons/Icons"; // Using stepper icon as closest to motor
import PwmMotorConfig from "./PwmMotorConfig";

export function PwmMotor(props: { id: string }) {
  const { state, error, setDutyCycle, stop } = createPwmMotorStore(props.id);
  const [currentDutyCycle, setCurrentDutyCycle] = createSignal(0);

  const debouncedSetDutyCycle = debounce((value: number) => {
    setDutyCycle(value);
  }, 100);

  return (
    <Device 
      id={props.id} 
      deviceState={state()} 
      configComponent={<PwmMotorConfig id={props.id} />}
      icon={<StepperIcon />}
    >
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${styles["device__status-indicator"]} ${
                state()?.running
                  ? styles["device__status-indicator--moving"]
                  : styles["device__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>
              {state()?.running
                ? `Running at ${state()?.dutyCycle?.toFixed(1) || 0}%`
                : `Stopped (${state()?.dutyCycle?.toFixed(1) || 0}%)`}
            </span>
            {state()?.running && (
              <button
                class={`${styles.device__button} ${styles["device__button--danger"]}`}
                onClick={() => stop()}
                style={{ "margin-left": "auto" }}
              >
                Stop
              </button>
            )}
          </div>

          <div class={styles.device__status}>
            <span class={styles["device__status-text"]}>
              Pin: {state()?.pin || "N/A"} | Channel: {state()?.pwmChannel || "N/A"} | Freq: {state()?.frequency || "N/A"}Hz
            </span>
          </div>

          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`dutyCycle-${props.id}`}>
              Duty Cycle: {(currentDutyCycle() || state()?.dutyCycle || 0).toFixed(1)}%
            </label>
            <input
              id={`dutyCycle-${props.id}`}
              class={styles.device__input}
              type="range"
              min="0"
              max="100"
              step="0.1"
              value={currentDutyCycle() || state()?.dutyCycle || 0}
              onInput={(e) => {
                const value = Number(e.currentTarget.value);
                setCurrentDutyCycle(value);
                debouncedSetDutyCycle(value);
              }}
            />
          </div>

          <div class={styles.device__controls}>
            <button
              class={styles.device__button}
              onClick={() => {
                setCurrentDutyCycle(0);
                setDutyCycle(0);
              }}
            >
              0%
            </button>
            <button
              class={styles.device__button}
              onClick={() => {
                setCurrentDutyCycle(25);
                setDutyCycle(25);
              }}
            >
              25%
            </button>
            <button
              class={styles.device__button}
              onClick={() => {
                setCurrentDutyCycle(50);
                setDutyCycle(50);
              }}
            >
              50%
            </button>
            <button
              class={styles.device__button}
              onClick={() => {
                setCurrentDutyCycle(75);
                setDutyCycle(75);
              }}
            >
              75%
            </button>
            <button
              class={styles.device__button}
              onClick={() => {
                setCurrentDutyCycle(100);
                setDutyCycle(100);
              }}
            >
              100%
            </button>
          </div>

          <div class={styles.device__controls}>
            <button
              class={`${styles.device__button} ${styles["device__button--danger"]}`}
              onClick={() => {
                setCurrentDutyCycle(0);
                stop();
              }}
            >
              Emergency Stop
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
