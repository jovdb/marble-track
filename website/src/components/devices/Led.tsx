import { Device } from "./Device";
import styles from "./Device.module.css";
import ledStyles from "./Led.module.css";
import { createLedStore } from "../../stores/Led";

export function Led(props: { id: string }) {
  const { state, error, setLed, blink } = createLedStore(props.id);

  return (
    <Device id={props.id} deviceState={state()}>
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${ledStyles["led__status-indicator"]} ${
                state()?.mode === "ON"
                  ? ledStyles["led__status-indicator--on"]
                  : state()?.mode === "BLINKING"
                    ? ledStyles["led__status-indicator--blinking"]
                    : ledStyles["led__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>Status: {state()?.mode}</span>
          </div>
          <div class={styles.device__controls}>
            <button class={styles.device__button} onClick={() => setLed(true)}>
              Turn On
            </button>
            <button
              class={`${styles.device__button} ${styles["device__button--secondary"]}`}
              onClick={() => setLed(false)}
            >
              Turn Off
            </button>
            <button
              class={`${styles.device__button} ${styles["device__button--secondary"]}`}
              onClick={() => blink?.()}
            >
              Blink
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
