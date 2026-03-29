import { type Component, Match, Switch } from "solid-js";
import type { ISystemInfoStore } from "../stores/SystemInfo";
import { Popup } from "./Popup";
import PopupHeader from "./PopupHeader";
import PopupContent from "./PopupContent";
import PopupFooter from "./PopupFooter";
import styles from "./SystemInfoPopup.module.css";

interface SystemInfoPopupProps {
  isOpen: boolean;
  onClose: () => void;
  systemInfo: ISystemInfoStore;
}

function formatUptime(uptimeMs?: number) {
  if (uptimeMs === undefined) {
    return "Unknown";
  }

  const totalSeconds = Math.floor(uptimeMs / 1000);
  const days = Math.floor(totalSeconds / 86400);
  const hours = Math.floor((totalSeconds % 86400) / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;

  const parts = [];
  if (days > 0) parts.push(`${days}d`);
  if (hours > 0 || days > 0) parts.push(`${hours}h`);
  if (minutes > 0 || hours > 0 || days > 0) parts.push(`${minutes}m`);
  parts.push(`${seconds}s`);
  return parts.join(" ");
}

const SystemInfoPopup: Component<SystemInfoPopupProps> = (props) => {
  return (
    <Popup isOpen={props.isOpen} onClose={props.onClose}>
      <PopupHeader title="ESP32 Information">
        <button
          onClick={props.onClose}
          style={{ background: "none", border: "none", "font-size": "24px", cursor: "pointer" }}
          aria-label="Close ESP32 information"
        >
          ×
        </button>
      </PopupHeader>

      <PopupContent>
        <Switch>
          <Match when={props.systemInfo.error}>
            <div class={styles["system-info-popup__error"]}>{props.systemInfo.error}</div>
          </Match>
          <Match when={!props.systemInfo.lastUpdatedAt}>
            <div class={styles["system-info-popup__loading"]}>Waiting for ESP32 information...</div>
          </Match>
          <Match when={true}>
            <dl class={styles["system-info-popup__list"]}>
              <div class={styles["system-info-popup__row"]}>
                <dt>Firmware build</dt>
                <dd>{props.systemInfo.firmwareBuild || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Hostname</dt>
                <dd>
                  {props.systemInfo.hostname ? `${props.systemInfo.hostname}.local` : "Unknown"}
                </dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>IP address</dt>
                <dd>{props.systemInfo.ipAddress || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Connection</dt>
                <dd>{props.systemInfo.connectionInfo || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Serial baud rate</dt>
                <dd>{props.systemInfo.serialBaudRate}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Free heap</dt>
                <dd>
                  {props.systemInfo.freeHeap !== undefined
                    ? `${props.systemInfo.freeHeap.toLocaleString()} bytes`
                    : "Unknown"}
                </dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Uptime</dt>
                <dd>{formatUptime(props.systemInfo.uptimeMs)}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Reset reason</dt>
                <dd>{props.systemInfo.resetReason || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>Chip model</dt>
                <dd>{props.systemInfo.chipModel || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>SDK version</dt>
                <dd>{props.systemInfo.sdkVersion || "Unknown"}</dd>
              </div>
              <div class={styles["system-info-popup__row"]}>
                <dt>WebSocket clients</dt>
                <dd>
                  {props.systemInfo.webSocketClients !== undefined
                    ? props.systemInfo.webSocketClients
                    : "Unknown"}
                </dd>
              </div>
            </dl>
          </Match>
        </Switch>
      </PopupContent>

      <PopupFooter>
        <button onClick={props.onClose}>Close</button>
      </PopupFooter>
    </Popup>
  );
};

export { SystemInfoPopup };
