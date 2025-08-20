import { Device, IDeviceState } from "./Device";
import { createDeviceState, IWsDeviceMessage, sendMessage } from "../../hooks/useWebSocket";
import styles from "./Device.module.css";

interface IButtonState extends IDeviceState {
  pressed: boolean;
}

export function Button(props: { id: string }) {
  const [deviceState, connectedState, disabled, error] = createDeviceState<IButtonState>(props.id);

  const handlePress = () => {
    sendMessage({
      type: "device-fn",
      deviceId: props.id,
      fn: "pressed",
    } as IWsDeviceMessage);
  };

  const handleRelease = () => {
    sendMessage({
      type: "device-fn",
      deviceId: props.id,
      fn: "released",
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
                deviceState()?.pressed
                  ? styles["device__status-indicator--pressed"]
                  : styles["device__status-indicator--off"]
              }`}
            ></div>
            <span class={styles["device__status-text"]}>
              Status: {deviceState()?.pressed ? "Pressed" : "Released"}
            </span>
          </div>
          <div class={styles.device__controls}>
            <button
              class={styles.device__button}
              onMouseDown={handlePress}
              onMouseUp={handleRelease}
              disabled={disabled()}
            >
              Hold to Press
            </button>
          </div>
        </>
      )}
    </Device>
  );
}
