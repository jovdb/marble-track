import { createMemo, createSignal, For, JSX, Show } from "solid-js";
import styles from "./Device.module.css";
import { useDevice } from "../../stores/Devices";
import { renderDeviceComponent } from "../Devices";
import { useWebSocket2 } from "../../hooks/useWebSocket2";
import { BroadcastIcon } from "../icons/Icons";

interface DeviceProps {
  id: string;
  icon?: JSX.Element;
  children?: JSX.Element | JSX.Element[];
  configComponent?: (onClose: () => void) => JSX.Element;
}

export function Device(props: DeviceProps) {
  const [showChildren, setShowChildren] = createSignal(false);
  const [showConfig, setShowConfig] = createSignal(false);
  const [showMessagesPanel, setShowMessages] = createSignal(false);
  const deviceStore = useDevice(props.id);
  const device = () => deviceStore[0];
  const [wsStore] = useWebSocket2();

  const name = createMemo(
    () =>
      (device()?.config as Record<string, string | undefined>)?.name ||
      device()?.id ||
      "Unknown Device"
  );
  const deviceType = createMemo(() => device()?.type);
  const hasConfig = createMemo(() => Boolean(props.configComponent));
  const configPanelId = `device-config-${props.id}`;
  const logsPanelId = `device-logs-${props.id}`;

  return (
    <div class={styles.device}>
      <div class={styles.device__header}>
        <div class={styles["device__header-left"]}>
          <Show when={props.icon}>
            <div class={styles.device__icon} aria-hidden="true">
              {props.icon}
            </div>
          </Show>
          <h3 class={styles.device__title}>{name()}</h3>
        </div>
        <div class={styles["device__header-right"]}>
          <div class={styles["device__header-actions"]}>
            <Show when={deviceType()}>
              <span class={styles["device__type-badge"]}>{deviceType()}</span>
            </Show>
            {device()?.children?.length && (
              <button
                class={`${styles["device__header-button"]} ${showChildren() ? styles["device__header-button--active"] : ""}`}
                type="button"
                aria-label={showChildren() ? "Hide advanced" : "Show advanced"}
                title={showChildren() ? "Hide children" : "Show children"}
                onClick={() => {
                  setShowChildren((v) => !v);
                  setShowConfig(false);
                  setShowMessages(false);
                }}
              >
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  width="24"
                  height="24"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  class="icon icon-tabler icons-tabler-outline icon-tabler-sitemap"
                >
                  <path stroke="none" d="M0 0h24v24H0z" fill="none" />
                  <path d="M3 15m0 2a2 2 0 0 1 2 -2h2a2 2 0 0 1 2 2v2a2 2 0 0 1 -2 2h-2a2 2 0 0 1 -2 -2z" />
                  <path d="M15 15m0 2a2 2 0 0 1 2 -2h2a2 2 0 0 1 2 2v2a2 2 0 0 1 -2 2h-2a2 2 0 0 1 -2 -2z" />
                  <path d="M9 3m0 2a2 2 0 0 1 2 -2h2a2 2 0 0 1 2 2v2a2 2 0 0 1 -2 2h-2a2 2 0 0 1 -2 -2z" />
                  <path d="M6 15v-1a2 2 0 0 1 2 -2h8a2 2 0 0 1 2 2v1" />
                  <path d="M12 9l0 3" />
                </svg>
              </button>
              // add config button
            )}
            <Show when={hasConfig()}>
              <button
                classList={{
                  [styles["device__header-button"]]: true,
                  [styles["device__header-button--active"]]: showConfig(),
                }}
                type="button"
                aria-expanded={showConfig()}
                aria-controls={configPanelId}
                aria-label={showConfig() ? "Hide configuration" : "Show configuration"}
                title={showConfig() ? "Hide configuration" : "Show configuration"}
                onClick={() => {
                  setShowConfig((v) => !v);
                  setShowChildren(false);
                  setShowMessages(false);
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
            </Show>
            <button
              classList={{
                [styles["device__header-button"]]: true,
                [styles["device__header-button--active"]]: showMessagesPanel(),
              }}
              type="button"
              aria-expanded={showMessagesPanel()}
              aria-controls={logsPanelId}
              aria-label={showMessagesPanel() ? "Hide messages" : "Show messages"}
              title={showMessagesPanel() ? "Hide messages" : "Show messages"}
              onClick={() => {
                setShowMessages((v) => !v);
                setShowChildren(false);
                setShowConfig(false);
              }}
            >
              <BroadcastIcon />
            </button>
          </div>
        </div>
      </div>
      <div class={styles.device__content}>
        <Show when={!showChildren() && !showConfig() && !showMessagesPanel()}>
          {props.children}
        </Show>

        {showChildren() && device()?.children?.length && (
          <div class={styles.device__children}>
            <For each={device()?.children}>{(child) => renderDeviceComponent(child)}</For>
          </div>
        )}
        <Show when={showConfig() && props.configComponent}>
          <div class={styles.device__children} id={configPanelId} role="region" aria-live="polite">
            {props.configComponent!(() => setShowConfig(false))}
          </div>
        </Show>
        <Show when={showMessagesPanel()}>
          <div class={styles.device__children} id={logsPanelId} role="region" aria-live="polite">
            <DeviceLogs deviceId={props.id} messages={wsStore.lastMessages} />
          </div>
        </Show>
      </div>
    </div>
  );
}

function DeviceLogs(props: { deviceId: string; messages: string[] }) {
  const filteredMessages = createMemo(() => {
    return props.messages
      .map((msgStr) => {
        try {
          const msg = JSON.parse(msgStr) as { data: string; direction: string; timestamp: number };
          const data = JSON.parse(msg.data);
          return { ...msg, parsed: data };
        } catch {
          return null;
        }
      })
      .filter(
        (msg): msg is NonNullable<typeof msg> =>
          msg !== null && msg.parsed.deviceId === props.deviceId
      )
      .reverse(); // Show newest first
  });

  return (
    <div>
      <h4>Device Logs</h4>
      <div
        style={{
          "max-height": "300px",
          "overflow-y": "auto",
          "font-family": "monospace",
          "font-size": "12px",
        }}
      >
        <For each={filteredMessages()}>
          {(msg) => (
            <div
              style={{
                margin: "4px 0",
                padding: "4px",
                border: "1px solid #ccc",
                "border-radius": "4px",
              }}
            >
              <div style={{ color: msg.direction === "incoming" ? "green" : "blue" }}>
                {msg.direction === "incoming" ? "←" : "→"} {msg.parsed.type}
              </div>
              <div>{JSON.stringify(msg.parsed, null, 2)}</div>
              <div style={{ "font-size": "10px", color: "#666" }}>
                {new Date(msg.timestamp).toLocaleTimeString()}
              </div>
            </div>
          )}
        </For>
        {filteredMessages().length === 0 && <div>No messages for this device</div>}
      </div>
    </div>
  );
}
