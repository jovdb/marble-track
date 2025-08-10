
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface IWheelState {
  position: number;
  name: string;
  type: string;
}

export function Wheel(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IWheelState>(props.id);

  const handleNext = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "goto",
        target: "next-breakpoint"
      })
    );
  };

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{deviceState()?.name || props.id}</h3>
        <span class={styles["device__type-badge"]}>WHEEL</span>
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
              <span class={styles["device__status-text"]}>
                Position: {deviceState()?.position}
              </span>
            </div>
            <div class={styles.device__controls}>
              <button 
                class={styles.device__button}
                onClick={handleNext}
                disabled={disabled()}
              >
                Next
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
