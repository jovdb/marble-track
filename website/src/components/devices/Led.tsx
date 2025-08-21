import { Device } from "./Device";
import styles from "./Device.module.css";
import { createLedStore } from "../../devices/Led";

export function Led(props: { id: string }) {
  const { state: deviceState, error, setLed } = createLedStore(props.id);

  return (
    <Device id={props.id} deviceState={deviceState()}>
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${styles["device__status-indicator"]} ${
                deviceState()?.mode === "ON"
                  ? styles["device__status-indicator--on"]
                  : styles["device__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>
              Status: {deviceState()?.mode === "ON" ? "On" : "Off"}
            </span>
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
          </div>
        </>
      )}
    </Device>
  );
}
