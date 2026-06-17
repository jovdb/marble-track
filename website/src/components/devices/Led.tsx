import { createMemo } from "solid-js";
import { Device } from "./Device";
import styles from "./Device.module.css";
import LedConfig from "./LedConfig";
import { getDeviceIcon } from "../icons/Icons";
import { useLed } from "../../stores/Led";
import { LedStateIcon } from "./LedStateIcon";

export function Led(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useLed(props.id);

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
      icon={deviceType ? getDeviceIcon(deviceType, props.id) : null}
      stateComponent={() => (
        <div
          style={{
            "padding-bottom": "24px",
            display: "flex",
            "justify-content": "center",
            "align-items": "center",
            height: "64px",
          }}
        >
          <LedStateIcon deviceId={props.id} width={64} height={64} />
        </div>
      )}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
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
