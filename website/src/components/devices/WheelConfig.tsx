import styles from "./Device.module.css";
import { createDeviceState, sendMessage } from "../../hooks/useWebSocket";
import { IWheelState } from "./Wheel";
// Update the import path below to the correct location of IWheelState

export function WheelConfig(props: { id: string }) {
  const [deviceState, connectedState, disabled] = createDeviceState<IWheelState>(props.id);

  const handleCalibrate = () => {
    sendMessage(
      JSON.stringify({
        type: "device-fn",
        deviceId: props.id,
        fn: "calibrate",
      })
    );
  };

  return (
    <div class={styles.device__controls}>
      <button class={styles.device__button} onClick={handleCalibrate} disabled={disabled()}>
        Calibrate
      </button>
    </div>
  );
}
