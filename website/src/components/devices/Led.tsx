import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { LedIcon } from "../icons/DeviceIcons";
import styles from "./Device.module.css";

interface ILedState {
  mode: "ON" | "OFF";
  pin: number;
  name: string;
  type: string;
}

export function Led(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<ILedState>(props.id);

  const setLed = (state: boolean) => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: state ? "on" : "off",
      })
    );
  };

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>
          <LedIcon />
          {deviceState()?.name || props.id}
        </h3>
        <span class={styles["device__type-badge"]}>LED</span>
      </div>
      
      <div class={styles.device__content}>
        {disabled() && (
          <div class={styles.device__error}>
            {error() || (connectedState() === "Disconnected" ? "Disconnected" : connectedState())}
          </div>
        )}
        
        {!disabled() && (
          <>
            <div class={styles.device__status}>
              <div class={`${styles["device__status-indicator"]} ${
                deviceState()?.mode === "ON" 
                  ? styles["device__status-indicator--on"] 
                  : styles["device__status-indicator--off"]
              }`}></div>
              <span class={styles["device__status-text"]}>
                Status: {deviceState()?.mode === "ON" ? "On" : "Off"}
              </span>
            </div>

            <div class={styles.device__controls}>
              <button 
                class={styles.device__button}
                onClick={() => setLed(true)} 
                disabled={disabled()}
              >
                Turn On
              </button>
              <button 
                class={`${styles.device__button} ${styles["device__button--secondary"]}`}
                onClick={() => setLed(false)} 
                disabled={disabled()}
              >
                Turn Off
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
