import { createMemo, onCleanup, onMount } from "solid-js";
import { Device } from "./Device";
import { PowerMonitorIcon } from "../icons/Icons";
import { usePowerMonitor } from "../../stores/PowerMonitor";
import PowerMonitorConfig from "./PowerMonitorConfig";
import styles from "./Device.module.css";

const POLL_INTERVAL_MS = 10_000;

export function PowerMonitor(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = usePowerMonitor(props.id);

  const state = createMemo(() => device()?.state);
  const status = createMemo(() => state()?.status);
  const voltage = createMemo(() => state()?.voltage);
  const current = createMemo(() => state()?.current);
  const watt = createMemo(() => state()?.watt);
  const timestamp = createMemo(() => state()?.timestamp);

  const requestState = () => actions.getDeviceState();

  // Poll every 10 s while the component is mounted
  onMount(() => {
    requestState();
    const id = setInterval(requestState, POLL_INTERVAL_MS);
    onCleanup(() => clearInterval(id));
  });

  const lastUpdated = createMemo(() => {
    const ts = timestamp();
    if (!ts) return "—";
    const secs = Math.round(ts / 1000);
    const m = Math.floor(secs / 60);
    const s = secs % 60;
    return m > 0 ? `${m}m ${s}s` : `${s}s`;
  });

  const statusText = createMemo(() => {
    const s = status();
    if (s === "Ready") return <span style={{ color: "green" }}>Ready</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>—</span>;
  });

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PowerMonitorConfig id={props.id} onClose={onClose} />}
      icon={<PowerMonitorIcon width={24} height={24} />}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Status:</strong> {statusText()}
        </div>
        <div>
          <strong>Voltage:</strong> {voltage() !== undefined ? `${voltage()!.toFixed(2)} V` : "—"}
        </div>
        <div>
          <strong>Current:</strong> {current() !== undefined ? `${current()!.toFixed(3)} A` : "—"}
        </div>
        <div>
          <strong>Power:</strong> {watt() !== undefined ? `${watt()!.toFixed(2)} W` : "—"}
        </div>
        <div style={{ "margin-top": "0.4rem", "font-size": "0.78rem", color: "#888" }}>
          <strong>Last read:</strong> {lastUpdated()} uptime
        </div>
      </div>

      <div class={styles.device__controls}>
        <button
          class={styles.device__button}
          onClick={requestState}
          title="Request a fresh measurement now"
        >
          Update
        </button>
      </div>
    </Device>
  );
}
