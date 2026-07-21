import { Device } from "./Device";
import { createMemo } from "solid-js";
import styles from "./Device.module.css";
import { useWheelLoader } from "../../stores/WheelLoader";
import { getDeviceIcon } from "../icons/Icons";

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
    >
      <div class={styles.device__status}>
        {state()?.state || "Unknown"}
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
