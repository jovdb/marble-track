import { Device } from "./Device";
import styles from "./Device.module.css";
import ledStyles from "./Led.module.css";
import LedConfig from "./LedConfig";
import { LedIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";

export function Led(props: { id: string }) {
  const [device, { setLed, blink }] = useLed(props.id);

  console.log("LED DEVICE", JSON.stringify(device, null, 2));
  return (
    <Device
      id={props.id}
      deviceState={device?.state}
      configComponent={<LedConfig id={props.id} />}
      icon={<LedIcon />}
    >
      <div class={styles.device__status}>
        <div
          class={`${ledStyles["led__status-indicator"]} ${
            device?.state?.mode === "ON"
              ? ledStyles["led__status-indicator--on"]
              : device?.state?.mode === "BLINKING"
                ? ledStyles["led__status-indicator--blinking"]
                : ledStyles["led__status-indicator--off"]
          }`}
        ></div>
        <span class={styles["device__status-text"]}>Status: {device?.state?.mode}</span>
      </div>
      <div class={styles.device__controls}>
        <button
          class={`${styles.device__button} ${device?.state?.mode === "ON" ? styles["device__button--secondary"] : ""}`}
          onClick={() => setLed(true)}
        >
          Turn On
        </button>
        <button
          class={`${styles.device__button} ${device?.state?.mode === "OFF" ? styles["device__button--secondary"] : ""}`}
          onClick={() => setLed(false)}
        >
          Turn Off
        </button>
        <button
          class={`${styles.device__button} ${device?.state?.mode === "BLINKING" ? styles["device__button--secondary"] : ""}`}
          onClick={() => blink()}
        >
          Blink
        </button>
      </div>
    </Device>
  );
}
