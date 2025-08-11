import { createSignal, JSX } from "solid-js";
import styles from "./Device.module.css";

export interface IDeviceState {
  name: string;
  type: string;
}

interface DeviceProps {
  id: string;
  name?: string;
  type?: string;
  hasChildren?: boolean;
  children?: JSX.Element | JSX.Element[];
}

export function Device(props: DeviceProps) {
  const [showChildren, setShowChildren] = createSignal(false);

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{props.name || props.id}</h3>
        {props.type && <span class={styles["device__type-badge"]}>{props.type}</span>}
        {props.hasChildren && (
          <button
            class={styles.device__button}
            type="button"
            onClick={() => setShowChildren((v) => !v)}
          >
            {showChildren() ? "Hide details" : "Show more"}
          </button>
        )}
      </div>
      <div class={styles.device__content}>
        {props.children}
        {props.hasChildren && showChildren() && (
          <div class={styles.device__children}>{props.children}</div>
        )}
      </div>
    </div>
  );
}
