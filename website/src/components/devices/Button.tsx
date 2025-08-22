import { Device } from "./Device";
import styles from "./Device.module.css";
import { createButtonStore } from "../../stores/Button";

export function Button(props: { id: string }) {
  const { state, error, press, release } = createButtonStore(props.id);

  const handlePress = () => {
    press();
  };

  const handleRelease = () => {
    release();
  };

  return (
    <Device id={props.id} deviceState={state()}>
      {error() && <div class={styles.device__error}>{error()}</div>}
      {!error() && (
        <>
          <div class={styles.device__status}>
            <div
              class={`${styles["device__status-indicator"]} ${
                state()?.pressed
                  ? styles["device__status-indicator--pressed"]
                  : styles["device__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>
              Status: {state()?.pressed ? "Pressed" : "Released"}
            </span>
          </div>
          <div class={styles.device__controls}>
            <button
              class={styles.device__button}
              onMouseDown={handlePress}
              onMouseUp={handleRelease}
            >
              Hold to Press
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
