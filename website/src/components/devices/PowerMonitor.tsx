import { Show } from "solid-js";
import { Device } from "./Device";
import { getDeviceIcon } from "../icons/Icons";
import { usePowerMonitor } from "../../stores/PowerMonitor";
import PowerMonitorConfig from "./PowerMonitorConfig";
import styles from "./Device.module.css";

function batteryColor(pct: number): string {
  if (pct >= 60) return "#4caf50"; // green
  if (pct >= 20) return "#ff9800"; // orange
  return "#f44336";                // red
}

export function PowerMonitor(props: { id: string; isPopup?: boolean; onClose?: () => void }) {
  const [device, actions] = usePowerMonitor(props.id);

  const config = () => device?.config;
  const state = () => device?.state;

  const status = () => state()?.status;
  const voltage = () => state()?.voltage;
  const current = () => state()?.current;
  const watt = () => state()?.watt;
  const timestamp = () => state()?.timestamp;

  const batteryPercent = () => {
    const v = voltage();
    const minV = config()?.minVoltage ?? 15.0;
    const maxV = config()?.maxVoltage ?? 21.0;
    if (v === undefined || maxV <= minV) return null;
    return Math.max(0, Math.min(100, ((v - minV) / (maxV - minV)) * 100));
  };

  const lastUpdated = () => {
    const ts = timestamp();
    if (!ts) return "—";
    const secs = Math.round(ts / 1000);
    const m = Math.floor(secs / 60);
    const s = secs % 60;
    return m > 0 ? `${m}m ${s}s` : `${s}s`;
  };

  const statusText = () => {
    const s = status();
    if (s === "Ready") return <span style={{ color: "green" }}>Ready</span>;
    if (s === "Error") return <span style={{ color: "red" }}>Error</span>;
    return <span>—</span>;
  };

  const pct = () => batteryPercent();

  return (
    <Device
      id={props.id}
      configComponent={(onClose) => <PowerMonitorConfig id={props.id} onClose={onClose} />}
      icon={device?.type ? getDeviceIcon(device.type, props.id) : null}
      isCollapsible={!props.isPopup}
      onClose={props.onClose}
    >
      <div style={{ padding: "0.5rem", "font-size": "0.9rem" }}>
        <div>
          <strong>Status:</strong> {statusText()}
        </div>
        <div>
          <strong>Voltage:</strong>{" "}
          {voltage() !== undefined ? `${voltage()!.toFixed(2)} V` : "—"}
        </div>
        <div>
          <strong>Current:</strong>{" "}
          {current() !== undefined ? `${current()!.toFixed(3)} A` : "—"}
        </div>
        <div>
          <strong>Power:</strong>{" "}
          {watt() !== undefined ? `${watt()!.toFixed(2)} W` : "—"}
        </div>

        {/* Battery percentage bar */}
        <Show when={pct() !== null}>
          <div style={{ "margin-top": "0.5rem" }}>
            <strong>Battery:</strong> {pct()!.toFixed(0)}%
            <div
              style={{
                "margin-top": "0.25rem",
                background: "#ddd",
                "border-radius": "4px",
                height: "10px",
                width: "100%",
                overflow: "hidden",
              }}
            >
              <div
                style={{
                  width: `${pct()!}%`,
                  height: "100%",
                  background: batteryColor(pct()!),
                  "border-radius": "4px",
                  transition: "width 0.4s ease, background 0.4s ease",
                }}
              />
            </div>
          </div>
        </Show>

        <div style={{ "margin-top": "0.4rem", "font-size": "0.78rem", color: "#888" }}>
          <strong>Last read:</strong> {lastUpdated()} uptime
        </div>
      </div>

      <div class={styles.device__controls}>
        <button
          class={styles.device__button}
          onClick={() => actions.init()}
          disabled={status() === "Ready"}
          title="Re-initialise the INA226 sensor"
        >
          Init
        </button>
      </div>
    </Device>
  );
}
