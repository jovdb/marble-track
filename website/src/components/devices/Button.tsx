import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface IButtonState {
  pressed: boolean;
  pin: number;
  name: string;
  type: string;
}

export function Button(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IButtonState>(props.id);

  const handlePress = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "pressed",
      })
    );
  };

  const handleRelease = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "released",
      })
    );
  };

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>
          🔘 {deviceState()?.name || props.id}
        </h3>
        <span class={styles["device__type-badge"]}>BUTTON</span>
      </div>
      
      <div class={styles.device__content}>
        {disabled() && (
          <div class={styles.device__error}>
            {error() || `Connection ${connectedState()}`}
          </div>
        )}
        
        {!disabled() && (
          <>
            <div class={styles.device__status}>
              <div class={`${styles["device__status-indicator"]} ${
                deviceState()?.pressed 
                  ? styles["device__status-indicator--pressed"] 
                  : styles["device__status-indicator--off"]
              }`}></div>
              <span class={styles["device__status-text"]}>
                Status: {deviceState()?.pressed ? "Pressed" : "Released"}
              </span>
            </div>

            <div class={styles.device__controls}>
              <button 
                class={styles.device__button}
                onMouseDown={handlePress}
                onMouseUp={handleRelease}
                onMouseLeave={handleRelease}
                disabled={disabled()}
              >
                Hold to Press
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
