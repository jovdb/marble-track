import { createMemo } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import ledStyles from "./Led.module.css";
import LedConfig from "./LedConfig";
import { LedIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";

export function Led(props: { id: string }) {
  const ledStore = useLed(props.id);
  const device = () => ledStore[0];
  const actions = ledStore[1];

  const mode = createMemo(() => device()?.state?.mode ?? "OFF");
  const statusClass = createMemo(() => {
    switch (mode()) {
      case "ON":
        return ledStyles["led__status-indicator--on"];
      case "BLINKING":
        return ledStyles["led__status-indicator--blinking"];
      default:
        return ledStyles["led__status-indicator--off"];
    }
  });
  const statusLabel = createMemo(() => `Status: ${mode()}`);
  const isMode = (value: string) => mode() === value;

  const handleTurnOn = () => actions.setLed(true);
  const handleTurnOff = () => actions.setLed(false);
  const handleBlink = () => actions.blink();

  return (
    <Device
      id={props.id}
      deviceState={device()?.state}
      configComponent={(onClose) => <LedConfig id={props.id} onClose={onClose} />}
      icon={<LedIcon />}
    >
      <div class={styles.device__status}>
        <div
          classList={{
            [ledStyles["led__status-indicator"]]: true,
            [statusClass()]: true,
          }}
          role="status"
          aria-label={statusLabel()}
        ></div>
        <span class={styles["device__status-text"]}>{statusLabel()}</span>
      </div>
      <div class={styles.device__controls}>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("ON"),
          }}
          disabled={isMode("ON")}
          onClick={handleTurnOn}
        >
          Turn On
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("OFF"),
          }}
          disabled={isMode("OFF")}
          onClick={handleTurnOff}
        >
          Turn Off
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("BLINKING"),
          }}
          disabled={isMode("BLINKING")}
          onClick={handleBlink}
        >
          Blink
        </button>
      </div>
    </Device>
  );
}
