import { createSignal, JSX } from "solid-js";
import styles from "./Device.module.css";

interface DeviceProps {
  id: string;
  name?: string;
  type?: string;
  hasChildren?: boolean;
  children?: JSX.Element | JSX.Element[];
}

export function Device({ id, name, type, hasChildren, children }: DeviceProps) {
  const [showChildren, setShowChildren] = createSignal(false);

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{name || id}</h3>
        {type && <span class={styles["device__type-badge"]}>{type}</span>}
        {hasChildren && (
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
        {children}
        {hasChildren && showChildren() && (
          <div class={styles.device__children}>{children}</div>
        )}
      </div>
    </div>
  );
}
