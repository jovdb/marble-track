import { Device } from "./Device";
import { createMemo, Show } from "solid-js";
import styles from "./Device.module.css";
import { useWheelLoader } from "../../stores/WheelLoader";
import { getDeviceIcon } from "../icons/Icons";
import WheelLoaderConfig from "./WheelLoaderConfig";

export function WheelLoader(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = useWheelLoader(props.id);

  const state = createMemo(() => device()?.state as any);
  const icon = createMemo(() => {
    const type = device()?.type;
    return type ? getDeviceIcon(type, props.id) : null;
  });

  return (
    <Device
      id={props.id}
      icon={icon()}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
      configComponent={(onClose) => <WheelLoaderConfig id={props.id} onClose={onClose} />}
    >
      <div class={styles.device__status}>
        <span class={styles["device__status-text"]}>{state()?.state || "Unknown"}</span>
        <span class={styles["device__status-text"]}>
          L:{state()?.leftBallAvailable ? "Yes" : "No"} R:{state()?.rightBallAvailable ? "Yes" : "No"}
        </span>
      </div>
      <div class={styles.device__controls}>
        <button class={styles.device__button} onClick={() => actions.init()}>
          Init
        </button>
        <button class={styles.device__button} onClick={() => actions.loadLeft()}>
          Load Left
        </button>
        <button class={styles.device__button} onClick={() => actions.loadRight()}>
          Load Right
        </button>
        <button class={styles.device__button} onClick={() => actions.loadAny()}>
          Load Any
        </button>
      </div>
    </Device>
  );
}
