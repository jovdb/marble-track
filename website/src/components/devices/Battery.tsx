import { onCleanup, onMount } from "solid-js";
import { Device } from "./Device";
import { BatteryIcon } from "../icons/Icons";
import { useBattery } from "../../stores/Battery";
import BatteryConfig from "./BatteryConfig";
import styles from "./Device.module.css";

const POLL_INTERVAL_MS = 60_000; // header-driven polls every 60 s

export function Battery(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, { getDeviceState }] = useBattery(props.id);

  const state = () => device?.state;
  const pct = () => state()?.batteryPercent ?? 0;
  const voltage = () => state()?.voltage;
  const status = () => state()?.status;
  const batteryLevel = () => Math.min(5, Math.max(0, Math.round(pct() / 20)));
  const requestState = getDeviceState;

  // Poll every 60 s for fresh state
  onMount(() => {
    requestState();
    const id = setInterval(requestState, POLL_INTERVAL_MS);
    onCleanup(() => clearInterval(id));
  });

  const statusText = () => {
    const s = status();
    if (s === "Ready") return <span style={{ color: "green" }}>Ready</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>—</span>;
  };

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <BatteryConfig id={props.id} onClose={onClose} />}
      icon={<BatteryIcon level={batteryLevel} width={24} height={24} />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0 0.5rem 0.5rem 0.5rem", "font-size": "0.9rem" }}>
        <div style={{ display: "flex", gap: "0.5rem" }}>
          <BatteryIcon level={batteryLevel} width="100%" height={80} />
        </div>

        <div>
          <strong>Status:</strong> {statusText()}
        </div>
        <div style={{ "margin-top": "0.5rem" }}>
          <strong>Battery:</strong> {pct().toFixed(0)}%
          {voltage() !== undefined && (
            <span style={{ "margin-left": "0.5rem", color: "#666", "font-size": "0.85rem" }}>
              ({voltage()!.toFixed(2)} V)
            </span>
          )}
        </div>

        {/* Percentage bar */}
        <div
          style={{
            "margin-top": "0.35rem",
            background: "#ddd",
            "border-radius": "4px",
            height: "12px",
            width: "100%",
            overflow: "hidden",
          }}
        >
          <div
            style={{
              width: `${pct()}%`,
              height: "100%",
              background: pct() >= 60 ? "#4caf50" : pct() >= 20 ? "#ff9800" : "#f44336",
              "border-radius": "4px",
              transition: "width 0.4s ease, background 0.4s ease",
            }}
          />
        </div>
      </div>
      <div class={styles.device__controls}>
        <button class={styles.device__button} onClick={requestState} title="Request updated value">
          Update
        </button>
      </div>
    </Device>
  );
}
