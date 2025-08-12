import { createSignal, For, JSX } from "solid-js";
import styles from "./Device.module.css";
import { renderDeviceComponent } from "../../App";

export interface IDeviceState {
  id: string;
  name: string;
  type: string;
  children?: IDeviceState[];
}

interface DeviceProps {
  id: string;
  deviceState: IDeviceState | undefined;
  children?: JSX.Element | JSX.Element[];
}

export function Device(props: DeviceProps) {
  const [showChildren, setShowChildren] = createSignal(false);

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{props.deviceState?.name || props.id}</h3>
        {props.deviceState?.type && (
          <span class={styles["device__type-badge"]}>{props.deviceState?.type}</span>
        )}
        {props.deviceState?.children?.length && (
          <button
            class={styles.device__advancedBtn}
            type="button"
            aria-label={showChildren() ? "Hide advanced" : "Show advanced"}
            onClick={() => setShowChildren((v) => !v)}
          >
            <svg
              width="24"
              height="24"
              viewBox="0 0 24 24"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <circle cx="5" cy="12" r="2" fill="currentColor" />
              <circle cx="12" cy="12" r="2" fill="currentColor" />
              <circle cx="19" cy="12" r="2" fill="currentColor" />
            </svg>
          </button>
        )}
      </div>
      <div class={styles.device__content}>
        {!showChildren && props.children}
        {props.deviceState?.children?.length && showChildren() && (
          <div class={styles.device__children}>
            <For each={props.deviceState.children}>
              {(child: IDeviceState) => renderDeviceComponent(child)}
            </For>
          </div>
        )}
      </div>
    </div>
  );
}
