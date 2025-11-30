import { createMemo, createSignal, For, JSX, Show } from "solid-js";
import styles from "./Device.module.css";
import { useDevice } from "../../stores/Devices";
import { renderDeviceComponent } from "../Devices";
import { useWebSocket2, IWebSocketMessage } from "../../hooks/useWebSocket";
import { BroadcastIcon } from "../icons/Icons";

interface DeviceProps {
  id: string;
  icon?: JSX.Element;
  children?: JSX.Element | JSX.Element[];
  configComponent?: (onClose: () => void) => JSX.Element;
}

export function Device(props: DeviceProps) {
  const [isCollapsed, setIsCollapsed] = createSignal(true);
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
  const hasError = createMemo(() => (device()?.state as any)?.errorCode > 0);
  const errorCode = createMemo(() => (device()?.state as any)?.errorCode || 0);
  const errorMessage = createMemo(() => (device()?.state as any)?.errorMessage || "");
  const configPanelId = `device-config-${props.id}`;
  const logsPanelId = `device-logs-${props.id}`;

  return (
    <div class={styles.device} data-device-id={props.id} id={`device-${props.id}`}>
      <div
        class={styles.device__header}
        onClick={() => {
          setIsCollapsed(!isCollapsed());
          setShowChildren(false);
          setShowConfig(false);
          setShowMessages(false);
        }}
      >
        <div class={styles["device__header-left"]}>
          <button
            class={styles["device__collapse-button"]}
            onClick={(e) => {
              e.stopPropagation();
              setIsCollapsed(!isCollapsed());
            }}
            aria-expanded={!isCollapsed()}
            aria-label={isCollapsed() ? "Expand device" : "Collapse device"}
            title={isCollapsed() ? "Expand device" : "Collapse device"}
          >
            <svg
              width="20"
              height="20"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              class={
                isCollapsed() ? styles["device__chevron--collapsed"] : styles["device__chevron"]
              }
            >
              <path d="M6 9l6 6 6-6" />
            </svg>
          </button>
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
            <Show when={hasError()}>
              <span 
                class={styles["device__error-badge"]}
                title={`Error ${errorCode()}: ${errorMessage()}`}
              >
                ⚠️ {errorCode()}
              </span>
            </Show>
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
                onClick={(e) => {
                  e.stopPropagation();
                  setShowConfig((v) => !v);
                  setShowChildren(false);
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
                  class="icon icon-tabler icons-tabler-outline icon-tabler-settings"
                >
                  <path stroke="none" d="M0 0h24v24H0z" fill="none" />
                  <path d="M10.325 4.317c.426 -1.756 2.924 -1.756 3.35 0a1.724 1.724 0 0 0 2.573 1.066c1.543 -.94 3.31 .826 2.37 2.37a1.724 1.724 0 0 0 1.065 2.572c1.756 .426 1.756 2.924 0 3.35a1.724 1.724 0 0 0 -1.066 2.573c.94 1.543 -.826 3.31 -2.37 2.37a1.724 1.724 0 0 0 -2.572 1.065c-.426 1.756 -2.924 1.756 -3.35 0a1.724 1.724 0 0 0 -2.573 -1.066c-1.543 .94 -3.31 -.826 -2.37 -2.37a1.724 1.724 0 0 0 -1.065 -2.572c-1.756 -.426 -1.756 -2.924 0 -3.35a1.724 1.724 0 0 0 1.066 -2.573c-.94 -1.543 .826 -3.31 2.37 -2.37c1 .608 2.296 .07 2.572 -1.065z" />
                  <path d="M9 12a3 3 0 1 0 6 0a3 3 0 0 0 -6 0" />
                </svg>
              </button>
            </Show>

            <Show when={device()?.children?.length}>
              <button
                class={`${styles["device__header-button"]} ${showChildren() ? styles["device__header-button--active"] : ""}`}
                type="button"
                aria-label={showChildren() ? "Hide advanced" : "Show advanced"}
                title={showChildren() ? "Hide children" : "Show children"}
                onClick={(e) => {
                  e.stopPropagation();
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
              onClick={(e) => {
                e.stopPropagation();
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
      <Show when={!isCollapsed()}>
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
            <div
              class={styles.device__children}
              id={configPanelId}
              role="region"
              aria-live="polite"
            >
              {props.configComponent!(() => setShowConfig(false))}
            </div>
          </Show>
          <Show when={showMessagesPanel()}>
            <div class={styles.device__children} id={logsPanelId} role="region" aria-live="polite">
              <DeviceLogs deviceId={props.id} messages={wsStore.lastMessages} />
            </div>
          </Show>
        </div>
      </Show>
    </div>
  );
}

function DeviceLogs(props: { deviceId: string; messages: IWebSocketMessage[] }) {
  const filteredMessages = createMemo(() => {
    return props.messages
      .map((msg) => {
        try {
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
      <h4>Messages</h4>
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
            </div>
          )}
        </For>
        {filteredMessages().length === 0 && <div>No messages for this device</div>}
      </div>
    </div>
  );
}
