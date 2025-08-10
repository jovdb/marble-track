
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface IGateState {
  gateState: "Closed" | "IsOpening" | "Opened" | "Closing";
  name: string;
  type: string;
}

export function Gate(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IGateState>(props.id);

  const openGate = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "open",
      })
    );
  };

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{deviceState()?.name || props.id}</h3>
        <span class={styles["device__type-badge"]}>GATE</span>
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
              <div class={`${styles["device__status-indicator"]} ${deviceState()?.gateState === "Opened" ? styles["device__status-indicator--on"] : styles["device__status-indicator--off"]}`}></div>
              <span class={styles["device__status-text"]}>
                Status: {deviceState()?.gateState || "Unknown"}
              </span>
            </div>
            <div class={styles.device__controls}>
              <button
                class={styles.device__button}
                onClick={openGate}
                disabled={disabled() || deviceState()?.gateState !== "Closed"}
              >
                Open
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
