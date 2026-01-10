import { createMemo } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import LedConfig from "./LedConfig";
import { getDeviceIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";
import { LedState } from "./LedState";

export function Led(props: { id: string }) {
  const ledStore = useLed(props.id);
  const device = () => ledStore[0];
  const actions = ledStore[1];

  const deviceType = device()?.type;
  const mode = createMemo(() => device()?.state?.mode ?? "");
  // Status visualization removed; use DeviceJsonState below
  const isMode = (value: string) => mode() === value;

  const handleTurnOn = () => actions.setLed(true);
  const handleTurnOff = () => actions.setLed(false);
  const handleBlink = () => actions.blink();

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <LedConfig id={props.id} onClose={onClose} />}
      icon={deviceType ? getDeviceIcon(deviceType) : null}
      stateComponent={() => <LedState id={props.id} />}
    >
      <div class={styles.device__controls}>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("ON"),
          }}
          disabled={!mode() || isMode("ON")}
          onClick={handleTurnOn}
        >
          Turn On
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("OFF"),
          }}
          disabled={!mode() || isMode("OFF")}
          onClick={handleTurnOff}
        >
          Turn Off
        </button>
        <button
          classList={{
            [styles.device__button]: true,
            [styles["device__button--secondary"]]: isMode("BLINKING"),
          }}
          disabled={!mode() || isMode("BLINKING")}
          onClick={handleBlink}
        >
          Blink
        </button>
      </div>
    </Device>
  );
}
