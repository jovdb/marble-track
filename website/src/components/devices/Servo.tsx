import { Device } from "./Device";
import { debounce } from "@solid-primitives/scheduled";
import { createSignal } from "solid-js";
import styles from "./Device.module.css";
import { createServoStore } from "../../stores/Servo";
import { ServoIcon } from "../icons/Icons";

export function Servo(props: { id: string }) {
  const { state, error, stop, setAngle } = createServoStore(props.id);
  const [currentSpeed, setCurrentSpeed] = createSignal(60);

  return (
    <Device id={props.id} deviceState={state()} icon={<ServoIcon />}>
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${styles["device__status-indicator"]} ${
                state()?.isMoving
                  ? styles["device__status-indicator--moving"]
                  : styles["device__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>
              {state()?.isMoving
                ? `Moving to ${state()?.targetAngle}\u00b0`
                : `At ${state()?.angle || 0}\u00b0`}
            </span>
            {state()?.isMoving && (
              <button
                class={`${styles.device__button} ${styles["device__button--danger"]}`}
                onClick={() => {
                  stop();
                }}
                style={{ "margin-left": "auto" }}
              >
                Stop
              </button>
            )}
          </div>
          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`angle-${props.id}`}>
              Angle: {state()?.angle || 0}\u00b0
            </label>
            <input
              id={`angle-${props.id}`}
              class={styles.device__input}
              type="range"
              min="0"
              max="180"
              value={state()?.angle || 90}
              onInput={(e) =>
                debounce(
                  () => setAngle({ angle: Number(e.currentTarget.value), speed: currentSpeed() }),
                  100
                )
              }
            />
            <div class={styles.device__controls}>
              <button
                class={styles.device__button}
                onClick={() => setAngle({ angle: 0, speed: currentSpeed() })}
              >
                0°
              </button>
              <button
                class={styles.device__button}
                onClick={() => setAngle({ angle: 45, speed: currentSpeed() })}
              >
                45°
              </button>
              <button
                class={styles.device__button}
                onClick={() => setAngle({ angle: 90, speed: currentSpeed() })}
              >
                90°
              </button>
              <button
                class={styles.device__button}
                onClick={() => setAngle({ angle: 135, speed: currentSpeed() })}
              >
                135°
              </button>
              <button
                class={styles.device__button}
                onClick={() => setAngle({ angle: 180, speed: currentSpeed() })}
              >
                180°
              </button>
            </div>
          </div>
          <div class={styles["device__input-group"]}>
            <label class={styles.device__label} for={`speed-${props.id}`}>
              Speed: {currentSpeed()}\u00b0/s
            </label>
            <input
              id={`speed-${props.id}`}
              class={styles.device__input}
              type="range"
              min="40"
              max="180"
              value={currentSpeed()}
              onInput={(e) => setCurrentSpeed(Number(e.currentTarget.value))}
            />
          </div>
        </>
      )}
    </Device>
  );
}
