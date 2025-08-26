import { createSignal, For, JSX } from "solid-js";
import styles from "./Device.module.css";
import { renderDeviceComponent } from "../../App";
import { IDeviceState } from "../../stores/Device";

interface DeviceProps {
  id: string;
  deviceState: IDeviceState | undefined;
  children?: JSX.Element | JSX.Element[];
  config?: JSX.Element;
}

export function Device(props: DeviceProps) {
  const [showChildren, setShowChildren] = createSignal(false);
  const [showConfig, setShowConfig] = createSignal(false);

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <h3 class={styles.device__title}>{props.deviceState?.name || props.id}</h3>
        <span style={{ display: "flex", gap: "var(--spacing-2)" }}>
          {props.deviceState?.type && (
            <span class={styles["device__type-badge"]}>{props.deviceState?.type}</span>
          )}
          {props.deviceState?.children?.length && (
            <button
              class={`${styles["device__header-button"]} ${showChildren() ? styles["device__header-button--active"] : ""}`}
              type="button"
              aria-label={showChildren() ? "Hide advanced" : "Show advanced"}
              onClick={() => {
                setShowChildren((v) => !v);
                setShowConfig(false);
              }}
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
            // add config button
          )}
          {!!props.config && (
            <button
              class={`${styles["device__header-button"]} ${showConfig() ? styles["device__header-button--active"] : ""}`}
              type="button"
              onClick={() => {
                setShowConfig((v) => !v);
                setShowChildren(false);
              }}
            >
              <svg
                fill="currentColor"
                width="24"
                height="24"
                viewBox="0 0 32 32"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path d="M31.449 6.748c-0.337-0.155-0.737-0.096-1.017 0.152l-5.041 4.528-4.551-4.669 4.506-5.204c0.245-0.283 0.305-0.673 0.152-1.016s-0.489-0.553-0.86-0.553h-0.271c-2.785 0-7.593 0.239-9.739 2.417l-0.433 0.43c-2.29 2.337-2.697 6.168-1.49 9.081l-11.54 11.778c-1.556 1.578-1.556 4.135 0 5.713l1.409 1.428c0.778 0.788 1.798 1.183 2.818 1.183s2.040-0.395 2.817-1.183l11.71-11.804c1.107 0.599 2.625 0.989 3.899 0.989 2.043 0 3.98-0.824 5.454-2.32l0.427-0.433c2.331-2.364 2.296-7.416 2.306-9.638 0.001-0.378-0.216-0.721-0.554-0.878zM28.302 15.906l-0.371 0.433c-1.117 1.134-2.578 1.677-4.114 1.677-0.76 0-1.784-0.143-2.476-0.431-0.625-0.259-1.206-0.634-1.725-1.107l-12.818 12.925c-0.376 0.382-0.876 0.592-1.408 0.592s-1.032-0.21-1.409-0.592l-1.408-1.427c-0.777-0.788-0.777-2.070-0.001-2.857l12.524-12.777c-0.42-0.611-0.706-1.278-0.877-1.968h-0.001c-0.482-1.95-0.201-4.644 1.313-6.189l0.431-0.435c1.298-1.317 4.67-1.707 6.537-1.822l-3.668 4.236c-0.328 0.379-0.311 0.95 0.038 1.309l5.798 5.948c0.352 0.362 0.92 0.383 1.299 0.047l4.082-3.676c-0.122 1.98-0.506 4.856-1.748 6.115z"></path>
              </svg>
            </button>
          )}
        </span>
      </div>
      <div class={styles.device__content}>
        {!showChildren() && !showConfig() && props.children}
        {showChildren() && props.deviceState?.children?.length && (
          <div class={styles.device__children}>
            <For each={props.deviceState.children}>
              {(child: IDeviceState) => renderDeviceComponent(child)}
            </For>
          </div>
        )}
        {showConfig() && props.config && <div class={styles.device__children}>{props.config}</div>}
      </div>
    </div>
  );
}
