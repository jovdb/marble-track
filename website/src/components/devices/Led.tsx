import { Device, IDeviceState } from "./Device";
import { createDeviceState, IWsDeviceMessage, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface ILedState extends IDeviceState {
  mode: "ON" | "OFF";
}

export function Led(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<ILedState>(props.id);

  const setLed = (state: boolean) => {
    sendMessage({
      type: "device-fn",
      deviceId: props.id,
      fn: state ? "on" : "off",
    } as IWsDeviceMessage);
  };

  return (
    <Device id={props.id} deviceState={deviceState()}>
      {disabled() && (
        <div class={styles.device__error}>
          {error() || (connectedState() === "Disconnected" ? "Disconnected" : connectedState())}
        </div>
      )}
      {!disabled() && (
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
    </Device>
  );
}
